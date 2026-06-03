/**
 * @file tests/bindings/test_c_ffi_contract.cpp
 * @brief yunlink FFI v1 contract checks.
 */

#include <array>
#include <cstring>
#include <cstddef>
#include <iostream>

#include "test_socket_utils.hpp"
#include "yunlink/c/yunlink_c.h"

namespace {

using yunlink::test_socket::SocketProtocol;

struct ContractPorts {
    uint16_t udp_bind{0};
    uint16_t tcp_listen{0};
};

bool allocate_contract_ports(ContractPorts* ports) {
    if (ports == nullptr) {
        return false;
    }

    const std::array<uint16_t, 2> empty_ports{};
    ports->udp_bind =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, empty_ports);
    if (ports->udp_bind == 0) {
        return false;
    }

    const std::array<uint16_t, 2> used_ports{ports->udp_bind, 0};
    ports->tcp_listen =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used_ports);
    return ports->tcp_listen != 0;
}

}  // namespace

int main() {
    static_assert(sizeof(yunlink_peer_t().id) == 128, "peer buffer contract changed");
    static_assert(sizeof(yunlink_runtime_config_t().shared_secret) == 64,
                  "shared secret buffer contract changed");
    static_assert(sizeof(yunlink_runtime_config_t().multicast_group) == 64,
                  "multicast group buffer contract changed");
    static_assert(sizeof(yunlink_error_event_t().message) == 256,
                  "error event message buffer contract changed");
    static_assert(sizeof(yunlink_command_result_event_t().detail) == 256,
                  "command result detail buffer contract changed");
    static_assert(sizeof(yunlink_session_info_t().node_name) == 128,
                  "session node_name buffer contract changed");
    static_assert(offsetof(yunlink_runtime_config_t, struct_size) == 0,
                  "runtime config struct_size must remain the first field");
    static_assert(offsetof(yunlink_target_selector_t, struct_size) == 0,
                  "target selector struct_size must remain the first field");
    static_assert(offsetof(yunlink_session_info_t, struct_size) == 0,
                  "session info struct_size must remain the first field");
    static_assert(offsetof(yunlink_target_selector_t, group_id) >
                      offsetof(yunlink_target_selector_t, entity_id),
                  "target selector field order changed");
    static_assert(offsetof(yunlink_command_handle_t, correlation_id) >
                      offsetof(yunlink_command_handle_t, message_id),
                  "command handle field order changed");
    static_assert(offsetof(yunlink_authority_lease_t, peer) >
                      offsetof(yunlink_authority_lease_t, expires_at_ms),
                  "authority lease field order changed");
    static_assert(offsetof(yunlink_runtime_event_t, data) > offsetof(yunlink_runtime_event_t, type),
                  "runtime event union field order changed");
    static_assert(offsetof(yunlink_command_result_event_t, detail) >
                      offsetof(yunlink_command_result_event_t, progress_percent),
                  "command result event field order changed");

    if (yunlink_runtime_create(nullptr) != YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "runtime_create(nullptr) mismatch\n";
        return 1;
    }
    if (std::strcmp(yunlink_result_name(YUNLINK_RESULT_CONNECT_ERROR),
                    "YUNLINK_RESULT_CONNECT_ERROR") != 0) {
        std::cerr << "result name mismatch\n";
        return 2;
    }

    yunlink_runtime_t* runtime = nullptr;
    if (yunlink_runtime_create(&runtime) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime create failed\n";
        return 3;
    }

    ContractPorts ports{};
    if (!allocate_contract_ports(&ports)) {
        std::cerr << "failed to allocate contract ports\n";
        yunlink_runtime_destroy(runtime);
        return 4;
    }

    yunlink_runtime_config_t cfg{};
    cfg.struct_size = sizeof(cfg);
    cfg.udp_bind_port = ports.udp_bind;
    cfg.udp_target_port = ports.udp_bind;
    cfg.tcp_listen_port = ports.tcp_listen;
    cfg.connect_timeout_ms = 5000;
    cfg.io_poll_interval_ms = 5;
    cfg.max_buffer_bytes_per_peer = 1 << 20;
    cfg.self_identity.agent_type = YUNLINK_AGENT_TYPE_GROUND_STATION;
    cfg.self_identity.agent_id = 99;
    cfg.self_identity.role = YUNLINK_ROLE_OBSERVER;
    std::strncpy(cfg.shared_secret, "yunlink-secret", sizeof(cfg.shared_secret) - 1);
    std::strncpy(cfg.multicast_group, "224.1.1.1", sizeof(cfg.multicast_group) - 1);

    if (yunlink_runtime_start(runtime, &cfg) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime start failed\n";
        yunlink_runtime_destroy(runtime);
        return 5;
    }

    yunlink_runtime_event_t event{};
    if (yunlink_runtime_poll_event(runtime, &event) != YUNLINK_RESULT_OK ||
        event.type != YUNLINK_RUNTIME_EVENT_NONE) {
        std::cerr << "empty poll mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 6;
    }

    yunlink_authority_lease_t lease{};
    if (yunlink_authority_current(runtime, &lease) != YUNLINK_RESULT_NOT_FOUND) {
        std::cerr << "authority current mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 7;
    }

    if (yunlink_peer_connect(nullptr, "127.0.0.1", ports.tcp_listen, nullptr) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "peer connect invalid argument mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 8;
    }

    yunlink_session_t invalid_session{};
    if (yunlink_session_open(runtime, nullptr, "ffi-ground", &invalid_session) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "session open invalid peer mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 9;
    }

    yunlink_peer_t fake_peer{};
    std::strncpy(fake_peer.id, "peer-1", sizeof(fake_peer.id) - 1);
    yunlink_target_selector_t invalid_target{};
    invalid_target.struct_size = 0;

    if (yunlink_authority_request(runtime,
                                  &fake_peer,
                                  &invalid_session,
                                  &invalid_target,
                                  YUNLINK_CONTROL_SOURCE_GROUND_STATION,
                                  3000,
                                  0) != YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "authority request invalid argument mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 10;
    }
    if (yunlink_authority_renew(runtime,
                                &fake_peer,
                                &invalid_session,
                                &invalid_target,
                                YUNLINK_CONTROL_SOURCE_GROUND_STATION,
                                3000) != YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "authority renew invalid argument mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 11;
    }
    if (yunlink_authority_release(runtime, &fake_peer, &invalid_session, &invalid_target) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "authority release invalid argument mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 12;
    }

    if (yunlink_runtime_stop(runtime) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime stop failed\n";
        yunlink_runtime_destroy(runtime);
        return 13;
    }
    if (yunlink_runtime_stop(runtime) != YUNLINK_RESULT_OK) {
        std::cerr << "repeated runtime stop failed\n";
        yunlink_runtime_destroy(runtime);
        return 14;
    }
    if (yunlink_peer_connect(runtime, "127.0.0.1", ports.tcp_listen, &fake_peer) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "runtime stopped mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 15;
    }

    yunlink_target_selector_t valid_target{};
    valid_target.struct_size = sizeof(valid_target);
    valid_target.scope = YUNLINK_TARGET_SCOPE_ENTITY;
    valid_target.target_type = YUNLINK_AGENT_TYPE_UAV;
    valid_target.entity_id = 1;

    yunlink_session_t stopped_session{};
    if (yunlink_session_open(runtime, &fake_peer, "stopped", &stopped_session) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "session open on stopped runtime mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 16;
    }

    stopped_session.session_id = 42;
    yunlink_goto_command_t goto_cmd{};
    goto_cmd.x_m = 1.0F;
    goto_cmd.y_m = 2.0F;
    goto_cmd.z_m = 3.0F;
    goto_cmd.yaw_rad = 0.1F;
    if (yunlink_command_publish_goto(
            runtime, &fake_peer, &stopped_session, &valid_target, &goto_cmd, nullptr) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "command publish on stopped runtime mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 17;
    }
    if (yunlink_authority_request(runtime,
                                  &fake_peer,
                                  &stopped_session,
                                  &valid_target,
                                  YUNLINK_CONTROL_SOURCE_GROUND_STATION,
                                  3000,
                                  0) != YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "authority request on stopped runtime mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 18;
    }

    yunlink_vehicle_core_state_t state{};
    state.armed = 1;
    state.battery_percent = 55.0F;
    if (yunlink_publish_vehicle_core_state(
            runtime, &fake_peer, &valid_target, &state, stopped_session.session_id) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "state publish on stopped runtime mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 19;
    }

    yunlink_runtime_event_t stopped_event{};
    if (yunlink_runtime_poll_event(runtime, &stopped_event) != YUNLINK_RESULT_OK ||
        stopped_event.type != YUNLINK_RUNTIME_EVENT_NONE) {
        std::cerr << "stopped runtime empty poll mismatch\n";
        yunlink_runtime_destroy(runtime);
        return 20;
    }

    yunlink_runtime_destroy(runtime);
    return 0;
}
