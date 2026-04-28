/**
 * @file tests/bindings/test_c_ffi_contract.cpp
 * @brief yunlink FFI v1 contract checks.
 */

#include <cstring>
#include <cstddef>
#include <iostream>

#include "yunlink/c/yunlink_c.h"

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

    yunlink_runtime_config_t cfg{};
    cfg.struct_size = sizeof(cfg);
    cfg.udp_bind_port = 13200;
    cfg.udp_target_port = 13200;
    cfg.tcp_listen_port = 13300;
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
        return 4;
    }

    yunlink_runtime_event_t event{};
    if (yunlink_runtime_poll_event(runtime, &event) != YUNLINK_RESULT_OK ||
        event.type != YUNLINK_RUNTIME_EVENT_NONE) {
        std::cerr << "empty poll mismatch\n";
        return 5;
    }

    yunlink_authority_lease_t lease{};
    if (yunlink_authority_current(runtime, &lease) != YUNLINK_RESULT_NOT_FOUND) {
        std::cerr << "authority current mismatch\n";
        return 6;
    }

    if (yunlink_peer_connect(nullptr, "127.0.0.1", 13300, nullptr) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "peer connect invalid argument mismatch\n";
        return 7;
    }

    yunlink_session_t invalid_session{};
    if (yunlink_session_open(runtime, nullptr, "ffi-ground", &invalid_session) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "session open invalid peer mismatch\n";
        return 8;
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
        return 9;
    }
    if (yunlink_authority_renew(runtime,
                                &fake_peer,
                                &invalid_session,
                                &invalid_target,
                                YUNLINK_CONTROL_SOURCE_GROUND_STATION,
                                3000) != YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "authority renew invalid argument mismatch\n";
        return 10;
    }
    if (yunlink_authority_release(runtime, &fake_peer, &invalid_session, &invalid_target) !=
        YUNLINK_RESULT_INVALID_ARGUMENT) {
        std::cerr << "authority release invalid argument mismatch\n";
        return 11;
    }

    if (yunlink_runtime_stop(runtime) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime stop failed\n";
        return 12;
    }
    if (yunlink_runtime_stop(runtime) != YUNLINK_RESULT_OK) {
        std::cerr << "repeated runtime stop failed\n";
        return 13;
    }
    if (yunlink_peer_connect(runtime, "127.0.0.1", 13300, &fake_peer) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "runtime stopped mismatch\n";
        return 14;
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
        return 15;
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
        return 16;
    }
    if (yunlink_authority_request(runtime,
                                  &fake_peer,
                                  &stopped_session,
                                  &valid_target,
                                  YUNLINK_CONTROL_SOURCE_GROUND_STATION,
                                  3000,
                                  0) != YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "authority request on stopped runtime mismatch\n";
        return 17;
    }

    yunlink_vehicle_core_state_t state{};
    state.armed = 1;
    state.battery_percent = 55.0F;
    if (yunlink_publish_vehicle_core_state(
            runtime, &fake_peer, &valid_target, &state, stopped_session.session_id) !=
        YUNLINK_RESULT_RUNTIME_STOPPED) {
        std::cerr << "state publish on stopped runtime mismatch\n";
        return 18;
    }

    yunlink_runtime_event_t stopped_event{};
    if (yunlink_runtime_poll_event(runtime, &stopped_event) != YUNLINK_RESULT_OK ||
        stopped_event.type != YUNLINK_RUNTIME_EVENT_NONE) {
        std::cerr << "stopped runtime empty poll mismatch\n";
        return 19;
    }

    yunlink_runtime_destroy(runtime);
    return 0;
}
