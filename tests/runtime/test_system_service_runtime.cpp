/**
 * @file tests/runtime/test_system_service_runtime.cpp
 * @brief System service request/response runtime test.
 */

#include <chrono>
#include <array>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "../bindings/test_socket_utils.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

using yunlink::test_socket::SocketProtocol;

bool wait_until(const std::function<bool()>& pred, int retries = 160, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

struct RuntimePorts {
    uint16_t server_udp_bind{0};
    uint16_t client_udp_bind{0};
    uint16_t server_tcp_listen{0};
    uint16_t client_tcp_listen{0};
};

bool allocate_runtime_ports(RuntimePorts* ports) {
    if (ports == nullptr) {
        return false;
    }

    std::array<uint16_t, 4> used_ports{};

    ports->server_udp_bind =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used_ports);
    if (ports->server_udp_bind == 0) {
        return false;
    }
    used_ports[0] = ports->server_udp_bind;

    ports->client_udp_bind =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used_ports);
    if (ports->client_udp_bind == 0) {
        return false;
    }
    used_ports[1] = ports->client_udp_bind;

    ports->server_tcp_listen =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used_ports);
    if (ports->server_tcp_listen == 0) {
        return false;
    }
    used_ports[2] = ports->server_tcp_listen;

    ports->client_tcp_listen =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used_ports);
    return ports->client_tcp_listen != 0;
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client;

    RuntimePorts ports{};
    if (!allocate_runtime_ports(&ports)) {
        std::cerr << "failed to allocate runtime ports\n";
        return 1;
    }

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = ports.server_udp_bind;
    server_cfg.udp_target_port = ports.server_udp_bind;
    server_cfg.tcp_listen_port = ports.server_tcp_listen;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 9;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "system-service-secret";

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = ports.client_udp_bind;
    client_cfg.udp_target_port = ports.client_udp_bind;
    client_cfg.tcp_listen_port = ports.client_tcp_listen;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 42;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_cfg.shared_secret = "system-service-secret";

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 2;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 3;
    }

    const uint64_t session_id =
        client.session_client().open_active_session(peer_id, "service-test");
    if (session_id == 0) {
        std::cerr << "open session failed\n";
        return 4;
    }
    if (!wait_until([&]() { return server.session_server().has_active_session(session_id); })) {
        std::cerr << "session did not become active\n";
        return 5;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9);
    bool saw_list_request = false;
    bool saw_get_request = false;
    bool saw_start_request = false;
    bool saw_stop_request = false;
    std::mutex mu;
    std::vector<yunlink::TypedMessage<yunlink::FeatureListResponse>> list_responses;
    std::vector<yunlink::TypedMessage<yunlink::FeatureGetResponse>> get_responses;
    std::vector<yunlink::TypedMessage<yunlink::FeatureStartResponse>> start_responses;
    std::vector<yunlink::TypedMessage<yunlink::FeatureStopResponse>> stop_responses;
    uint64_t list_request_message_id = 0;
    uint64_t get_request_message_id = 0;
    uint64_t start_request_message_id = 0;
    uint64_t stop_request_message_id = 0;

    const size_t list_req_token =
        server.system_service_subscriber().subscribe_feature_list_requests(
            [&](const yunlink::InboundSystemServiceRequestView<yunlink::FeatureListRequest>& view) {
                yunlink::FeatureListResponse response{};
                response.success = true;
                response.message = "ok";
                response.feature_names = {"single_uav_basic", "mapping"};
                if (server.system_service_publisher().publish_feature_list_response(
                        view.inbound, response) != yunlink::ErrorCode::kOk) {
                    std::cerr << "publish feature list response failed\n";
                }
                saw_list_request = true;
                list_request_message_id = view.inbound.envelope.message_id;
            });
    const size_t get_req_token = server.system_service_subscriber().subscribe_feature_get_requests(
        [&](const yunlink::InboundSystemServiceRequestView<yunlink::FeatureGetRequest>& view) {
            yunlink::FeatureGetResponse response{};
            response.success = true;
            response.message = "ok";
            response.name = view.payload.feature_name;
            response.title = "Single UAV Basic";
            response.group = "单机无人机";
            response.running = true;
            response.description = "单机无人机基础链路";
            response.auto_start = false;
            response.depends_on = {"localization"};
            response.start_preview_units = {"localization", "uav_control"};
            response.start_preview_commands = {"roslaunch localization_fusion ..."};
            if (server.system_service_publisher().publish_feature_get_response(
                    view.inbound, response) != yunlink::ErrorCode::kOk) {
                std::cerr << "publish feature get response failed\n";
            }
            saw_get_request = true;
            get_request_message_id = view.inbound.envelope.message_id;
        });
    const size_t start_req_token =
        server.system_service_subscriber().subscribe_feature_start_requests(
            [&](const yunlink::InboundSystemServiceRequestView<yunlink::FeatureStartRequest>&
                    view) {
                yunlink::FeatureStartResponse response{};
                response.success = true;
                response.message = "feature started";
                response.feature_name = view.payload.feature_name;
                if (server.system_service_publisher().publish_feature_start_response(
                        view.inbound, response) != yunlink::ErrorCode::kOk) {
                    std::cerr << "publish feature start response failed\n";
                }
                saw_start_request = true;
                start_request_message_id = view.inbound.envelope.message_id;
            });
    const size_t stop_req_token =
        server.system_service_subscriber().subscribe_feature_stop_requests(
            [&](const yunlink::InboundSystemServiceRequestView<yunlink::FeatureStopRequest>& view) {
                yunlink::FeatureStopResponse response{};
                response.success = !view.payload.force;
                response.message = view.payload.force ? "force stop requested" : "feature stopped";
                response.feature_name = view.payload.feature_name;
                if (server.system_service_publisher().publish_feature_stop_response(
                        view.inbound, response) != yunlink::ErrorCode::kOk) {
                    std::cerr << "publish feature stop response failed\n";
                }
                saw_stop_request = true;
                stop_request_message_id = view.inbound.envelope.message_id;
            });

    const size_t list_resp_token =
        client.system_service_subscriber().subscribe_feature_list_responses(
            [&](const yunlink::TypedMessage<yunlink::FeatureListResponse>& view) {
                std::lock_guard<std::mutex> lock(mu);
                list_responses.push_back(view);
            });
    const size_t get_resp_token =
        client.system_service_subscriber().subscribe_feature_get_responses(
            [&](const yunlink::TypedMessage<yunlink::FeatureGetResponse>& view) {
                std::lock_guard<std::mutex> lock(mu);
                get_responses.push_back(view);
            });
    const size_t start_resp_token =
        client.system_service_subscriber().subscribe_feature_start_responses(
            [&](const yunlink::TypedMessage<yunlink::FeatureStartResponse>& view) {
                std::lock_guard<std::mutex> lock(mu);
                start_responses.push_back(view);
            });
    const size_t stop_resp_token =
        client.system_service_subscriber().subscribe_feature_stop_responses(
            [&](const yunlink::TypedMessage<yunlink::FeatureStopResponse>& view) {
                std::lock_guard<std::mutex> lock(mu);
                stop_responses.push_back(view);
            });

    yunlink::SystemServiceHandle list_handle{};
    yunlink::FeatureListRequest list_request{};
    if (client.system_service_publisher().publish_feature_list_request(
            peer_id, session_id, target, list_request, &list_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish feature list request failed\n";
        return 6;
    }

    yunlink::SystemServiceHandle get_handle{};
    yunlink::FeatureGetRequest get_request{};
    get_request.feature_name = "single_uav_basic";
    if (client.system_service_publisher().publish_feature_get_request(
            peer_id, session_id, target, get_request, &get_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish feature get request failed\n";
        return 7;
    }

    yunlink::SystemServiceHandle start_handle{};
    yunlink::FeatureStartRequest start_request{};
    start_request.feature_name = "single_uav_basic";
    start_request.override_args = {"use_sim:=true"};
    start_request.restart_if_running = true;
    start_request.start_with_terminal = false;
    if (client.system_service_publisher().publish_feature_start_request(
            peer_id, session_id, target, start_request, &start_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish feature start request failed\n";
        return 8;
    }

    yunlink::SystemServiceHandle stop_handle{};
    yunlink::FeatureStopRequest stop_request{};
    stop_request.feature_name = "single_uav_basic";
    stop_request.force = true;
    if (client.system_service_publisher().publish_feature_stop_request(
            peer_id, session_id, target, stop_request, &stop_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish feature stop request failed\n";
        return 9;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return saw_list_request && saw_get_request && saw_start_request && saw_stop_request &&
                   list_responses.size() == 1 && get_responses.size() == 1 &&
                   start_responses.size() == 1 && stop_responses.size() == 1;
        })) {
        std::cerr << "system service roundtrip not observed\n";
        return 10;
    }

    server.system_service_subscriber().unsubscribe(list_req_token);
    server.system_service_subscriber().unsubscribe(get_req_token);
    server.system_service_subscriber().unsubscribe(start_req_token);
    server.system_service_subscriber().unsubscribe(stop_req_token);
    client.system_service_subscriber().unsubscribe(list_resp_token);
    client.system_service_subscriber().unsubscribe(get_resp_token);
    client.system_service_subscriber().unsubscribe(start_resp_token);
    client.system_service_subscriber().unsubscribe(stop_resp_token);

    client.stop();
    server.stop();

    std::lock_guard<std::mutex> lock(mu);
    if (!saw_list_request || !saw_get_request || !saw_start_request || !saw_stop_request) {
        std::cerr << "request handler not invoked\n";
        return 11;
    }
    if (list_responses.front().envelope.correlation_id != list_request_message_id ||
        list_responses.front().envelope.correlation_id != list_handle.message_id) {
        std::cerr << "feature list correlation mismatch\n";
        return 12;
    }
    if (!list_responses.front().payload.success ||
        list_responses.front().payload.feature_names.size() != 2 ||
        list_responses.front().payload.feature_names.front() != "single_uav_basic") {
        std::cerr << "feature list response payload mismatch\n";
        return 13;
    }
    if (get_responses.front().envelope.correlation_id != get_request_message_id ||
        get_responses.front().envelope.correlation_id != get_handle.message_id) {
        std::cerr << "feature get correlation mismatch\n";
        return 14;
    }
    if (!get_responses.front().payload.success ||
        get_responses.front().payload.name != "single_uav_basic" ||
        get_responses.front().payload.start_preview_units.size() != 2 ||
        get_responses.front().payload.depends_on.size() != 1) {
        std::cerr << "feature get response payload mismatch\n";
        return 15;
    }
    if (start_responses.front().envelope.correlation_id != start_request_message_id ||
        start_responses.front().envelope.correlation_id != start_handle.message_id) {
        std::cerr << "feature start correlation mismatch\n";
        return 16;
    }
    if (!start_responses.front().payload.success ||
        start_responses.front().payload.feature_name != "single_uav_basic") {
        std::cerr << "feature start response payload mismatch\n";
        return 17;
    }
    if (stop_responses.front().envelope.correlation_id != stop_request_message_id ||
        stop_responses.front().envelope.correlation_id != stop_handle.message_id) {
        std::cerr << "feature stop correlation mismatch\n";
        return 18;
    }
    if (stop_responses.front().payload.success ||
        stop_responses.front().payload.feature_name != "single_uav_basic" ||
        stop_responses.front().payload.message != "force stop requested") {
        std::cerr << "feature stop response payload mismatch\n";
        return 19;
    }

    return 0;
}
