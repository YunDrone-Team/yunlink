/** @file @brief C ABI managed-entity callback ownership and nested-view contract. */

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "test_socket_utils.hpp"
#include "yunlink/c/yunlink_c.h"
#include "yunlink/runtime/runtime.hpp"

namespace {

using yunlink::test_socket::SocketProtocol;

bool wait_until(const std::function<bool()>& predicate) {
    for (int attempt = 0; attempt < 300; ++attempt) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

std::string copy_view(yunlink_string_view_t view) {
    return view.data == nullptr ? std::string{} : std::string(view.data, view.size);
}

yunlink::EndpointIdentity uav(uint32_t id) {
    yunlink::EndpointIdentity identity;
    identity.agent_type = yunlink::AgentType::kUav;
    identity.agent_id = id;
    identity.role = yunlink::EndpointRole::kVehicle;
    identity.group_ids = {7, 9};
    return identity;
}

struct CallbackState {
    std::atomic<bool> called{false};
    uint64_t correlation_id{0};
    std::string endpoint_uid;
    std::string entity_uid;
    std::string display_name;
    std::string capability;
    std::vector<uint32_t> group_ids;
};

void on_directory(void* user_data, const yunlink_managed_entity_list_response_view_t* response) {
    auto* state = static_cast<CallbackState*>(user_data);
    if (state == nullptr || response == nullptr || response->entity_count != 2 ||
        response->entities == nullptr) {
        return;
    }
    const auto& second = response->entities[1];
    if (second.capability_count == 0 || second.capabilities == nullptr) {
        return;
    }
    state->correlation_id = response->correlation_id;
    state->endpoint_uid = copy_view(response->endpoint_uid);
    state->entity_uid = copy_view(second.entity_uid);
    state->display_name = copy_view(second.display_name);
    state->capability = copy_view(second.capabilities[0]);
    state->group_ids.assign(second.identity.group_ids,
                            second.identity.group_ids + second.identity.group_id_count);
    state->called.store(true);
}

}  // namespace

int main() {
    const std::array<uint16_t, 4> empty{};
    const uint16_t server_udp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, empty);
    const std::array<uint16_t, 4> used1{server_udp, 0, 0, 0};
    const uint16_t client_udp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used1);
    const std::array<uint16_t, 4> used2{server_udp, client_udp, 0, 0};
    const uint16_t server_tcp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used2);
    const std::array<uint16_t, 4> used3{server_udp, client_udp, server_tcp, 0};
    const uint16_t client_tcp =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used3);
    if (server_udp == 0 || client_udp == 0 || server_tcp == 0 || client_tcp == 0) {
        return 1;
    }

    yunlink::Runtime server;
    yunlink::RuntimeConfig server_config;
    server_config.udp_bind_port = server_udp;
    server_config.udp_target_port = server_udp;
    server_config.tcp_listen_port = server_tcp;
    server_config.self_identity = uav(1);
    server_config.shared_secret = "ffi-managed-secret";
    if (server.start(server_config) != yunlink::ErrorCode::kOk) {
        return 2;
    }

    const size_t request_token =
        server.system_service_subscriber().subscribe_managed_entity_list_requests(
            [&](const yunlink::InboundSystemServiceRequestView<yunlink::ManagedEntityListRequest>&
                    request) {
                yunlink::ManagedEntityListResponse response;
                response.success = true;
                response.message = "ok";
                response.endpoint_uid = "endpoint-ffi-managed";
                response.revision = "revision-42";
                response.primary_identity = uav(1);
                for (uint32_t id : {1U, 2U}) {
                    yunlink::ManagedEntityDescriptor descriptor;
                    descriptor.entity_uid = "opaque-uav-" + std::to_string(id);
                    descriptor.identity = uav(id);
                    descriptor.display_name = "UAV " + std::to_string(id);
                    descriptor.hardware_id = "SIM-" + std::to_string(id);
                    descriptor.capabilities = {"telemetry.px4", "control.uav"};
                    descriptor.availability = yunlink::ManagedEntityAvailability::kOnline;
                    response.entities.push_back(std::move(descriptor));
                }
                (void)server.system_service_publisher().publish_managed_entity_list_response(
                    request.inbound, response);
            });

    yunlink_runtime_t* client = nullptr;
    if (yunlink_runtime_create(&client) != YUNLINK_RESULT_OK) {
        return 3;
    }
    yunlink_runtime_config_t config{};
    config.struct_size = sizeof(config);
    config.udp_bind_port = client_udp;
    config.udp_target_port = client_udp;
    config.tcp_listen_port = client_tcp;
    config.connect_timeout_ms = 5000;
    config.io_poll_interval_ms = 5;
    config.max_buffer_bytes_per_peer = 1 << 20;
    config.self_identity.agent_type = YUNLINK_AGENT_TYPE_GROUND_STATION;
    config.self_identity.agent_id = 1001;
    config.self_identity.role = YUNLINK_ROLE_CONTROLLER;
    std::strncpy(config.shared_secret, "ffi-managed-secret", sizeof(config.shared_secret) - 1);
    std::strncpy(config.multicast_group, "224.1.1.1", sizeof(config.multicast_group) - 1);
    if (yunlink_runtime_start(client, &config) != YUNLINK_RESULT_OK) {
        return 4;
    }

    yunlink_peer_t peer{};
    yunlink_session_t session{};
    if (yunlink_peer_connect(client, "127.0.0.1", server_tcp, &peer) != YUNLINK_RESULT_OK ||
        yunlink_session_open(client, &peer, "ffi-managed", &session) != YUNLINK_RESULT_OK ||
        !wait_until(
            [&]() { return server.session_server().has_active_session(session.session_id); })) {
        return 5;
    }

    CallbackState state;
    size_t callback_token = 0;
    if (yunlink_system_service_subscribe_managed_entity_list_responses(
            client, on_directory, &state, &callback_token) != YUNLINK_RESULT_OK) {
        return 6;
    }
    yunlink_target_selector_t target{};
    target.struct_size = sizeof(target);
    target.scope = YUNLINK_TARGET_SCOPE_ENTITY;
    target.target_type = YUNLINK_AGENT_TYPE_UAV;
    target.entity_id = 1;
    yunlink_command_handle_t handle{};
    if (yunlink_system_service_request_managed_entity_list(
            client, &peer, &session, &target, &handle) != YUNLINK_RESULT_OK ||
        !wait_until([&]() { return state.called.load(); })) {
        return 7;
    }
    if (state.correlation_id != handle.message_id || state.endpoint_uid != "endpoint-ffi-managed" ||
        state.entity_uid != "opaque-uav-2" || state.display_name != "UAV 2" ||
        state.capability != "telemetry.px4" || state.group_ids != std::vector<uint32_t>({7, 9})) {
        return 8;
    }

    if (yunlink_system_service_unsubscribe(client, callback_token) != YUNLINK_RESULT_OK) {
        return 9;
    }
    server.system_service_subscriber().unsubscribe(request_token);
    yunlink_runtime_destroy(client);
    server.stop();
    return 0;
}
