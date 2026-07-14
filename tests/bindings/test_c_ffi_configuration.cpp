/** @file @brief C ABI configuration resource callback-view contract. */

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "test_socket_utils.hpp"
#include "yunlink/c/yunlink_c.h"
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

struct CallbackState {
    std::atomic<bool> called{false};
    uint64_t correlation_id = 0;
    std::string resource_id;
    std::string title;
};

std::string copy_view(yunlink_string_view_t view) {
    return view.data == nullptr ? std::string() : std::string(view.data, view.size);
}

void on_list_response(void* user_data,
                      const yunlink_config_resource_list_response_view_t* response) {
    auto* state = static_cast<CallbackState*>(user_data);
    if (response == nullptr || response->status != YUNLINK_CONFIG_STATUS_OK ||
        response->resource_count != 1) {
        return;
    }
    state->correlation_id = response->correlation_id;
    state->resource_id = copy_view(response->resources[0].id);
    state->title = copy_view(response->resources[0].title);
    state->called = true;
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
    server_config.self_identity.agent_type = yunlink::AgentType::kUav;
    server_config.self_identity.agent_id = 7;
    server_config.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_config.shared_secret = "ffi-config-secret";
    if (server.start(server_config) != yunlink::ErrorCode::kOk) {
        return 2;
    }

    const size_t server_token =
        server.configuration_service_subscriber().subscribe_resource_list_requests(
            [&](const yunlink::InboundConfigurationServiceRequestView<
                yunlink::ConfigResourceListRequest>& request) {
                yunlink::ConfigResourceListResponse response;
                response.status = yunlink::ConfigServiceStatus::kOk;
                response.message = "ok";
                response.resources.push_back(
                    {"sunray.device.identity", "Device identity", "Identity", true, true, true});
                (void)server.configuration_service_publisher().publish_resource_list_response(
                    request.inbound, response);
            });

    yunlink_runtime_t* client = nullptr;
    if (yunlink_runtime_create(&client) != YUNLINK_RESULT_OK) {
        return 3;
    }
    yunlink_runtime_config_t client_config{};
    client_config.struct_size = sizeof(client_config);
    client_config.udp_bind_port = client_udp;
    client_config.udp_target_port = client_udp;
    client_config.tcp_listen_port = client_tcp;
    client_config.connect_timeout_ms = 5000;
    client_config.io_poll_interval_ms = 5;
    client_config.max_buffer_bytes_per_peer = 1 << 20;
    client_config.self_identity.agent_type = YUNLINK_AGENT_TYPE_GROUND_STATION;
    client_config.self_identity.agent_id = 1001;
    client_config.self_identity.role = YUNLINK_ROLE_CONTROLLER;
    std::strncpy(
        client_config.shared_secret, "ffi-config-secret", sizeof(client_config.shared_secret) - 1);
    std::strncpy(
        client_config.multicast_group, "224.1.1.1", sizeof(client_config.multicast_group) - 1);
    if (yunlink_runtime_start(client, &client_config) != YUNLINK_RESULT_OK) {
        return 4;
    }

    yunlink_peer_t peer{};
    yunlink_session_t session{};
    if (yunlink_peer_connect(client, "127.0.0.1", server_tcp, &peer) != YUNLINK_RESULT_OK ||
        yunlink_session_open(client, &peer, "ffi-config", &session) != YUNLINK_RESULT_OK ||
        !wait_until(
            [&]() { return server.session_server().has_active_session(session.session_id); })) {
        return 5;
    }

    CallbackState callback_state;
    size_t callback_token = 0;
    if (yunlink_configuration_subscribe_resource_list_responses(
            client, on_list_response, &callback_state, &callback_token) != YUNLINK_RESULT_OK ||
        callback_token == 0) {
        return 6;
    }

    yunlink_target_selector_t target{};
    target.struct_size = sizeof(target);
    target.scope = YUNLINK_TARGET_SCOPE_ENTITY;
    target.target_type = YUNLINK_AGENT_TYPE_UAV;
    target.entity_id = 7;
    yunlink_configuration_handle_t handle{};
    if (yunlink_configuration_publish_resource_list_request(
            client, &peer, &session, &target, &handle) != YUNLINK_RESULT_OK ||
        handle.message_id == 0 || !wait_until([&]() { return callback_state.called.load(); })) {
        return 7;
    }

    if (callback_state.correlation_id != handle.message_id ||
        callback_state.resource_id != "sunray.device.identity" ||
        callback_state.title != "Device identity") {
        return 8;
    }
    if (yunlink_configuration_unsubscribe(client, callback_token) != YUNLINK_RESULT_OK) {
        return 9;
    }

    server.configuration_service_subscriber().unsubscribe(server_token);
    yunlink_runtime_destroy(client);
    server.stop();
    return 0;
}
