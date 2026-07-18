/** @file @brief Topic directory, subscription and directed sample runtime test. */

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "../bindings/test_socket_utils.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& predicate, int retries = 160, int sleep_ms = 20) {
    for (int index = 0; index < retries; ++index) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

struct Ports {
    uint16_t server_udp = 0;
    uint16_t client_udp = 0;
    uint16_t server_tcp = 0;
    uint16_t client_tcp = 0;
};

bool allocate_ports(Ports* ports) {
    if (ports == nullptr) {
        return false;
    }
    std::array<uint16_t, 4> used{};
    ports->server_udp = yunlink::test_socket::find_unique_free_port(
        yunlink::test_socket::SocketProtocol::kUdp, used);
    used[0] = ports->server_udp;
    ports->client_udp = yunlink::test_socket::find_unique_free_port(
        yunlink::test_socket::SocketProtocol::kUdp, used);
    used[1] = ports->client_udp;
    ports->server_tcp = yunlink::test_socket::find_unique_free_port(
        yunlink::test_socket::SocketProtocol::kTcp, used);
    used[2] = ports->server_tcp;
    ports->client_tcp = yunlink::test_socket::find_unique_free_port(
        yunlink::test_socket::SocketProtocol::kTcp, used);
    return ports->server_udp != 0 && ports->client_udp != 0 && ports->server_tcp != 0 &&
           ports->client_tcp != 0;
}

}  // namespace

int main() {
    Ports ports{};
    if (!allocate_ports(&ports)) {
        std::cerr << "failed to allocate ports\n";
        return 1;
    }

    yunlink::Runtime server;
    yunlink::Runtime client;
    yunlink::RuntimeConfig server_config{};
    server_config.udp_bind_port = ports.server_udp;
    server_config.udp_target_port = ports.server_udp;
    server_config.tcp_listen_port = ports.server_tcp;
    server_config.self_identity =
        {yunlink::AgentType::kUav, 7, yunlink::EndpointRole::kVehicle};
    server_config.shared_secret = "topic-runtime-secret";
    yunlink::RuntimeConfig client_config{};
    client_config.udp_bind_port = ports.client_udp;
    client_config.udp_target_port = ports.client_udp;
    client_config.tcp_listen_port = ports.client_tcp;
    client_config.self_identity =
        {yunlink::AgentType::kGroundStation, 42, yunlink::EndpointRole::kController};
    client_config.shared_secret = server_config.shared_secret;

    if (server.start(server_config) != yunlink::ErrorCode::kOk ||
        client.start(client_config) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 2;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", ports.server_tcp, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 3;
    }
    const uint64_t session_id = client.session_client().open_active_session(peer_id, "topics");
    if (session_id == 0 ||
        !wait_until([&]() { return server.session_server().has_active_session(session_id); })) {
        std::cerr << "session did not become active\n";
        return 4;
    }

    std::atomic<bool> saw_list_request{false};
    std::atomic<bool> saw_subscription_request{false};
    std::mutex mutex;
    yunlink::TypedMessage<yunlink::TopicListResponse> list_response{};
    yunlink::TypedMessage<yunlink::TopicSubscriptionResponse> subscription_response{};
    yunlink::TypedMessage<yunlink::TopicSample> received_sample{};
    bool has_list_response = false;
    bool has_subscription_response = false;
    bool has_sample = false;
    bool has_uav_control = false;
    yunlink::UavControlCommand received_uav_control{};

    const size_t list_request_token =
        server.system_service_subscriber().subscribe_topic_list_requests(
            [&](const yunlink::InboundSystemServiceRequestView<yunlink::TopicListRequest>& view) {
                saw_list_request.store(true);
                yunlink::TopicListResponse response{};
                response.success = true;
                response.message = "ok";
                response.revision = "revision-1";
                response.topics.push_back(
                    {"/uav7/sunray/px4_state", "sunray_msgs/Px4State", 1});
                (void)server.system_service_publisher().publish_topic_list_response(view.inbound,
                                                                                   response);
            });
    const size_t subscription_request_token =
        server.system_service_subscriber().subscribe_topic_subscription_requests(
            [&](const yunlink::InboundSystemServiceRequestView<
                yunlink::TopicSubscriptionRequest>& view) {
                saw_subscription_request.store(true);
                yunlink::TopicSubscriptionResponse response{};
                response.success = true;
                response.message = "subscribed";
                response.topic_name = view.payload.topic_name;
                response.subscribed = view.payload.subscribe;
                response.type_name = "sunray_msgs/Px4State";
                response.max_rate_hz = view.payload.max_rate_hz;
                response.max_payload_bytes = view.payload.max_payload_bytes;
                (void)server.system_service_publisher().publish_topic_subscription_response(
                    view.inbound, response);

                yunlink::TopicSample sample{};
                sample.topic_name = view.payload.topic_name;
                sample.type_name = response.type_name;
                sample.type_hash = "hash";
                sample.encoding = "ros1";
                sample.message_definition = "uint8 state\n";
                sample.receive_time_ns = 100;
                sample.sequence = 1;
                sample.metadata_included = true;
                sample.data = {7};
                const auto target = yunlink::TargetSelector::for_entity(
                    view.inbound.envelope.source.agent_type,
                    view.inbound.envelope.source.agent_id);
                (void)server.publish_topic_sample(view.inbound.peer.id,
                                                  target,
                                                  sample,
                                                  view.inbound.envelope.session_id);
            });
    const size_t list_response_token =
        client.system_service_subscriber().subscribe_topic_list_responses(
            [&](const yunlink::TypedMessage<yunlink::TopicListResponse>& response) {
                std::lock_guard<std::mutex> lock(mutex);
                list_response = response;
                has_list_response = true;
            });
    const size_t subscription_response_token =
        client.system_service_subscriber().subscribe_topic_subscription_responses(
            [&](const yunlink::TypedMessage<yunlink::TopicSubscriptionResponse>& response) {
                std::lock_guard<std::mutex> lock(mutex);
                subscription_response = response;
                has_subscription_response = true;
            });
    const size_t sample_token = client.state_subscriber().subscribe_topic_samples(
        [&](const yunlink::TypedMessage<yunlink::TopicSample>& sample) {
            std::lock_guard<std::mutex> lock(mutex);
            received_sample = sample;
            has_sample = true;
        });
    const size_t command_token = server.command_subscriber().subscribe_uav_control(
        [&](const yunlink::InboundCommandView<yunlink::UavControlCommand>& command) {
            std::lock_guard<std::mutex> lock(mutex);
            received_uav_control = command.payload;
            has_uav_control = true;
        });

    const auto target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 7);
    yunlink::SystemServiceHandle list_handle{};
    if (client.system_service_publisher().publish_topic_list_request(
            peer_id, session_id, target, {}, &list_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "topic list request failed\n";
        return 5;
    }
    yunlink::TopicSubscriptionRequest request{};
    request.topic_name = "/uav7/sunray/px4_state";
    request.subscribe = true;
    request.max_rate_hz = 10.0F;
    request.max_payload_bytes = 262144;
    yunlink::SystemServiceHandle subscription_handle{};
    if (client.system_service_publisher().publish_topic_subscription_request(
            peer_id, session_id, target, request, &subscription_handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "topic subscription request failed\n";
        return 6;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            return saw_list_request.load() && saw_subscription_request.load() &&
                   has_list_response && has_subscription_response && has_sample;
        })) {
        std::cerr << "topic runtime exchange incomplete\n";
        return 7;
    }

    if (client.request_authority(peer_id,
                                 session_id,
                                 target,
                                 yunlink::ControlSource::kGroundStation,
                                 3000) != yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 8;
    }
    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return server.current_authority_for_target(target, &lease); })) {
        std::cerr << "authority was not granted\n";
        return 9;
    }
    yunlink::UavControlCommand command{};
    command.control_cmd = 4;
    command.desired_position_m = {1.0F, 2.0F, 3.0F};
    command.yaw_mode = 1;
    command.desired_yaw_rad = 0.5F;
    command.controller_type = 2;
    if (client.command_publisher().publish_uav_control(
            peer_id, session_id, target, command) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            return has_uav_control;
        })) {
        std::cerr << "unified UAV control command was not delivered\n";
        return 10;
    }

    server.system_service_subscriber().unsubscribe(list_request_token);
    server.system_service_subscriber().unsubscribe(subscription_request_token);
    client.system_service_subscriber().unsubscribe(list_response_token);
    client.system_service_subscriber().unsubscribe(subscription_response_token);
    client.state_subscriber().unsubscribe(sample_token);
    server.command_subscriber().unsubscribe(command_token);
    client.stop();
    server.stop();

    std::lock_guard<std::mutex> lock(mutex);
    if (!list_response.payload.success || list_response.payload.topics.size() != 1 ||
        list_response.envelope.correlation_id != list_handle.message_id ||
        !subscription_response.payload.subscribed ||
        subscription_response.envelope.correlation_id != subscription_handle.message_id ||
        received_sample.payload.data != yunlink::ByteBuffer({7}) ||
        received_sample.envelope.session_id != session_id ||
        received_sample.envelope.qos_class != yunlink::QosClass::kReliableOrdered ||
        received_uav_control.control_cmd != command.control_cmd ||
        received_uav_control.desired_position_m.z != command.desired_position_m.z ||
        received_uav_control.controller_type != command.controller_type) {
        std::cerr << "topic runtime metadata mismatch\n";
        return 11;
    }
    return 0;
}
