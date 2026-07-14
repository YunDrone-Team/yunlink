/**
 * @file tests/runtime/test_configuration_service_runtime.cpp
 * @brief Configuration service request/response runtime contract.
 */

#include <array>
#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>

#include "../bindings/test_socket_utils.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

using yunlink::test_socket::SocketProtocol;

bool wait_until(const std::function<bool()>& predicate, int retries = 160) {
    for (int index = 0; index < retries; ++index) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

struct RuntimePorts {
    uint16_t server_udp = 0;
    uint16_t client_udp = 0;
    uint16_t server_tcp = 0;
    uint16_t client_tcp = 0;
};

bool allocate_ports(RuntimePorts* ports) {
    std::array<uint16_t, 4> used{};
    ports->server_udp = yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used);
    used[0] = ports->server_udp;
    ports->client_udp = yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used);
    used[1] = ports->client_udp;
    ports->server_tcp = yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used);
    used[2] = ports->server_tcp;
    ports->client_tcp = yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used);
    return ports->server_udp != 0 && ports->client_udp != 0 && ports->server_tcp != 0 &&
           ports->client_tcp != 0;
}

}  // namespace

int main() {
    RuntimePorts ports{};
    if (!allocate_ports(&ports)) {
        return 1;
    }

    yunlink::Runtime server;
    yunlink::Runtime client;
    yunlink::RuntimeConfig server_config;
    server_config.udp_bind_port = ports.server_udp;
    server_config.udp_target_port = ports.server_udp;
    server_config.tcp_listen_port = ports.server_tcp;
    server_config.self_identity.agent_type = yunlink::AgentType::kUav;
    server_config.self_identity.agent_id = 9;
    server_config.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_config.shared_secret = "configuration-service-secret";

    yunlink::RuntimeConfig client_config;
    client_config.udp_bind_port = ports.client_udp;
    client_config.udp_target_port = ports.client_udp;
    client_config.tcp_listen_port = ports.client_tcp;
    client_config.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_config.self_identity.agent_id = 42;
    client_config.self_identity.role = yunlink::EndpointRole::kController;
    client_config.shared_secret = server_config.shared_secret;

    if (server.start(server_config) != yunlink::ErrorCode::kOk ||
        client.start(client_config) != yunlink::ErrorCode::kOk) {
        return 2;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", ports.server_tcp, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        return 3;
    }
    const uint64_t session_id = client.session_client().open_active_session(peer_id, "config-test");
    if (session_id == 0 ||
        !wait_until([&]() { return server.session_server().has_active_session(session_id); })) {
        return 4;
    }

    std::mutex mutex;
    bool request_seen = false;
    bool response_seen = false;
    uint64_t request_message_id = 0;
    yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse> received{};

    const size_t request_token =
        server.configuration_service_subscriber().subscribe_resource_patch_requests(
            [&](const yunlink::InboundConfigurationServiceRequestView<
                yunlink::ConfigResourcePatchRequest>& view) {
                yunlink::ConfigResourcePatchResponse response{};
                response.status = yunlink::ConfigServiceStatus::kOk;
                response.message = "stored";
                response.snapshot.resource_id = view.payload.resource_id;
                response.snapshot.revision = "sha256:after";
                response.snapshot.applied_revision = "sha256:before";
                response.snapshot.values = view.payload.updates;
                response.effects.requirement = yunlink::ConfigApplyRequirement::kComponentRestart;
                response.effects.affected_components = {"bridge"};
                response.effects.reconnect_expected = true;
                if (server.configuration_service_publisher().publish_resource_patch_response(
                        view.inbound, response) != yunlink::ErrorCode::kOk) {
                    return;
                }
                std::lock_guard<std::mutex> lock(mutex);
                request_seen = true;
                request_message_id = view.inbound.envelope.message_id;
            });

    const size_t response_token =
        client.configuration_service_subscriber().subscribe_resource_patch_responses(
            [&](const yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse>& message) {
                std::lock_guard<std::mutex> lock(mutex);
                received = message;
                response_seen = true;
            });

    yunlink::ConfigResourcePatchRequest request{};
    request.resource_id = "vendor.device.identity";
    request.expected_revision = "sha256:before";
    request.updates.push_back({"agent_id", yunlink::ConfigValue::from_int64(8)});
    request.validate_only = false;
    yunlink::ConfigurationServiceHandle handle{};
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9);
    if (client.configuration_service_publisher().publish_resource_patch_request(
            peer_id, session_id, target, request, &handle) != yunlink::ErrorCode::kOk) {
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mutex);
            return request_seen && response_seen;
        })) {
        return 6;
    }

    server.configuration_service_subscriber().unsubscribe(request_token);
    client.configuration_service_subscriber().unsubscribe(response_token);
    client.stop();
    server.stop();

    std::lock_guard<std::mutex> lock(mutex);
    if (received.envelope.qos_class != yunlink::QosClass::kReliableOrdered ||
        received.envelope.message_family != yunlink::MessageFamily::kConfigurationService ||
        received.envelope.correlation_id != request_message_id ||
        received.envelope.correlation_id != handle.message_id) {
        return 7;
    }
    if (received.payload.status != yunlink::ConfigServiceStatus::kOk ||
        received.payload.snapshot.revision != "sha256:after" ||
        received.payload.snapshot.values.size() != 1 ||
        !received.payload.effects.reconnect_expected) {
        return 8;
    }
    return 0;
}
