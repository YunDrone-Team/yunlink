/** @file @brief Optional security tags preserve capability negotiation compatibility. */

#include <array>
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

#include "../bindings/test_socket_utils.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

using yunlink::test_socket::SocketProtocol;

bool wait_until(const std::function<bool()>& predicate) {
    for (int index = 0; index < 160; ++index) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    return false;
}

}  // namespace

int main() {
    std::array<uint16_t, 4> used{};
    const uint16_t server_udp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used);
    used[0] = server_udp;
    const uint16_t client_udp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used);
    used[1] = client_udp;
    const uint16_t server_tcp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used);
    used[2] = server_tcp;
    const uint16_t client_tcp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used);

    yunlink::Runtime server;
    yunlink::Runtime client;
    yunlink::RuntimeConfig server_config;
    server_config.udp_bind_port = server_udp;
    server_config.udp_target_port = server_udp;
    server_config.tcp_listen_port = server_tcp;
    server_config.self_identity = {yunlink::AgentType::kUav, 1, yunlink::EndpointRole::kVehicle};
    server_config.shared_secret = "optional-tags";
    server_config.security_tags_enabled = true;

    yunlink::RuntimeConfig client_config;
    client_config.udp_bind_port = client_udp;
    client_config.udp_target_port = client_udp;
    client_config.tcp_listen_port = client_tcp;
    client_config.self_identity = {
        yunlink::AgentType::kGroundStation, 1001, yunlink::EndpointRole::kController};
    client_config.shared_secret = server_config.shared_secret;
    client_config.security_tags_enabled = true;

    if (server.start(server_config) != yunlink::ErrorCode::kOk ||
        client.start(client_config) != yunlink::ErrorCode::kOk) {
        return 1;
    }
    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_tcp, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        return 2;
    }
    const uint64_t session_id = client.session_client().open_active_session(peer_id, "optional");
    if (session_id == 0 ||
        !wait_until([&]() { return server.session_server().has_active_session(session_id); })) {
        return 3;
    }

    std::atomic<bool> signed_request_seen{false};
    const size_t token = server.configuration_service_subscriber().subscribe_resource_list_requests(
        [&](const yunlink::InboundConfigurationServiceRequestView<
            yunlink::ConfigResourceListRequest>& request) {
            signed_request_seen = !request.inbound.envelope.security.auth_tag.empty();
        });
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (client.configuration_service_publisher().publish_resource_list_request(
            peer_id, session_id, target, {}) != yunlink::ErrorCode::kOk ||
        !wait_until([&]() { return signed_request_seen.load(); })) {
        return 4;
    }
    if (server_config.capability_flags != 0 || client_config.capability_flags != 0) {
        return 5;
    }
    server.configuration_service_subscriber().unsubscribe(token);
    client.stop();
    server.stop();
    return 0;
}
