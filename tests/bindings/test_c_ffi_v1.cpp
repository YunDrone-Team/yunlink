/**
 * @file tests/bindings/test_c_ffi_v1.cpp
 * @brief yunlink FFI v1 runtime loop test.
 */

#include <chrono>
#include <array>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include "test_socket_utils.hpp"
#include "yunlink/c/yunlink_c.h"

namespace {

using yunlink::test_socket::SocketProtocol;

struct RuntimePorts {
    uint16_t air_udp_bind{0};
    uint16_t ground_udp_bind{0};
    uint16_t air_tcp_listen{0};
    uint16_t ground_tcp_listen{0};
};

bool wait_until(const std::function<bool()>& pred, int retries = 120, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

bool allocate_runtime_ports(RuntimePorts* ports) {
    if (ports == nullptr) {
        return false;
    }

    std::array<uint16_t, 4> used_ports{};

    ports->air_udp_bind =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used_ports);
    if (ports->air_udp_bind == 0) {
        return false;
    }
    used_ports[0] = ports->air_udp_bind;

    ports->ground_udp_bind =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kUdp, used_ports);
    if (ports->ground_udp_bind == 0) {
        return false;
    }
    used_ports[1] = ports->ground_udp_bind;

    ports->air_tcp_listen =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used_ports);
    if (ports->air_tcp_listen == 0) {
        return false;
    }
    used_ports[2] = ports->air_tcp_listen;

    ports->ground_tcp_listen =
        yunlink::test_socket::find_unique_free_port(SocketProtocol::kTcp, used_ports);
    if (ports->ground_tcp_listen == 0) {
        return false;
    }
    used_ports[3] = ports->ground_tcp_listen;

    return true;
}

yunlink_runtime_config_t make_config(uint16_t udp_bind,
                                     uint16_t udp_target,
                                     uint16_t tcp_listen,
                                     uint8_t agent_type,
                                     uint32_t agent_id,
                                     uint8_t role) {
    yunlink_runtime_config_t cfg{};
    cfg.struct_size = sizeof(cfg);
    cfg.udp_bind_port = udp_bind;
    cfg.udp_target_port = udp_target;
    cfg.tcp_listen_port = tcp_listen;
    cfg.connect_timeout_ms = 5000;
    cfg.io_poll_interval_ms = 5;
    cfg.max_buffer_bytes_per_peer = 1 << 20;
    cfg.self_identity.agent_type = agent_type;
    cfg.self_identity.agent_id = agent_id;
    cfg.self_identity.role = role;
    cfg.capability_flags = 0;
    std::strncpy(cfg.shared_secret, "yunlink-secret", sizeof(cfg.shared_secret) - 1);
    std::strncpy(cfg.multicast_group, "224.1.1.1", sizeof(cfg.multicast_group) - 1);
    return cfg;
}

bool start_runtimes_with_retry(yunlink_runtime_t* air,
                               yunlink_runtime_t* ground,
                               yunlink_runtime_config_t* air_cfg,
                               yunlink_runtime_config_t* ground_cfg,
                               int attempts = 8) {
    if (air == nullptr || ground == nullptr || air_cfg == nullptr || ground_cfg == nullptr) {
        return false;
    }

    for (int i = 0; i < attempts; ++i) {
        RuntimePorts ports{};
        if (!allocate_runtime_ports(&ports)) {
            continue;
        }

        *air_cfg = make_config(ports.air_udp_bind,
                               ports.air_udp_bind,
                               ports.air_tcp_listen,
                               YUNLINK_AGENT_TYPE_UAV,
                               1,
                               YUNLINK_ROLE_VEHICLE);
        *ground_cfg = make_config(ports.ground_udp_bind,
                                  ports.ground_udp_bind,
                                  ports.ground_tcp_listen,
                                  YUNLINK_AGENT_TYPE_GROUND_STATION,
                                  7,
                                  YUNLINK_ROLE_CONTROLLER);

        const auto air_result = yunlink_runtime_start(air, air_cfg);
        const auto ground_result = yunlink_runtime_start(ground, ground_cfg);
        if (air_result == YUNLINK_RESULT_OK && ground_result == YUNLINK_RESULT_OK) {
            return true;
        }

        yunlink_runtime_stop(ground);
        yunlink_runtime_stop(air);
    }

    return false;
}

}  // namespace

int main() {
    if (yunlink_ffi_abi_version() == 0) {
        std::cerr << "ffi abi version missing\n";
        return 1;
    }
    if (std::strcmp(yunlink_result_name(YUNLINK_RESULT_OK), "YUNLINK_RESULT_OK") != 0) {
        std::cerr << "result name lookup mismatch\n";
        return 2;
    }

    yunlink_runtime_t* air = nullptr;
    yunlink_runtime_t* ground = nullptr;
    if (yunlink_runtime_create(&air) != YUNLINK_RESULT_OK ||
        yunlink_runtime_create(&ground) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime create failed\n";
        return 3;
    }

    yunlink_runtime_config_t air_cfg{};
    yunlink_runtime_config_t ground_cfg{};
    if (!start_runtimes_with_retry(air, ground, &air_cfg, &ground_cfg)) {
        std::cerr << "runtime start failed after port retries\n";
        yunlink_runtime_destroy(ground);
        yunlink_runtime_destroy(air);
        return 4;
    }
    if (yunlink_runtime_start(ground, &ground_cfg) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime repeated start failed\n";
        yunlink_runtime_stop(ground);
        yunlink_runtime_stop(air);
        yunlink_runtime_destroy(ground);
        yunlink_runtime_destroy(air);
        return 5;
    }

    yunlink_peer_t air_peer{};
    if (yunlink_peer_connect(ground, "127.0.0.1", air_cfg.tcp_listen_port, &air_peer) !=
        YUNLINK_RESULT_OK) {
        std::cerr << "peer connect failed\n";
        return 6;
    }

    yunlink_session_t session{};
    if (yunlink_session_open(ground, &air_peer, "ffi-ground", &session) != YUNLINK_RESULT_OK ||
        session.session_id == 0) {
        std::cerr << "session open failed\n";
        return 7;
    }
    yunlink_session_info_t session_info{};
    session_info.struct_size = sizeof(session_info);
    if (!wait_until([&]() {
            return yunlink_session_describe(air, &session, &session_info) == YUNLINK_RESULT_OK &&
                   session_info.session_id == session.session_id && session_info.state == 5 &&
                   session_info.remote_identity.agent_type == YUNLINK_AGENT_TYPE_GROUND_STATION;
        })) {
        std::cerr << "typed session describe did not observe active session\n";
        return 20;
    }

    const yunlink_target_selector_t target{
        sizeof(yunlink_target_selector_t),
        YUNLINK_TARGET_SCOPE_ENTITY,
        YUNLINK_AGENT_TYPE_UAV,
        1,
        0,
    };
    const yunlink_target_selector_t ground_target{
        sizeof(yunlink_target_selector_t),
        YUNLINK_TARGET_SCOPE_ENTITY,
        YUNLINK_AGENT_TYPE_GROUND_STATION,
        7,
        0,
    };

    if (yunlink_authority_request(
            ground, &air_peer, &session, &target, YUNLINK_CONTROL_SOURCE_GROUND_STATION, 3000, 0) !=
        YUNLINK_RESULT_OK) {
        std::cerr << "authority request failed\n";
        return 8;
    }

    yunlink_authority_lease_t lease{};
    if (!wait_until([&]() {
            return yunlink_authority_current(air, &lease) == YUNLINK_RESULT_OK &&
                   lease.session_id == session.session_id &&
                   lease.state == YUNLINK_AUTHORITY_STATE_CONTROLLER;
        })) {
        std::cerr << "authority lease not observed\n";
        return 9;
    }
    if (yunlink_authority_renew(
            ground, &air_peer, &session, &target, YUNLINK_CONTROL_SOURCE_GROUND_STATION, 4500) !=
        YUNLINK_RESULT_OK) {
        std::cerr << "authority renew failed\n";
        return 10;
    }
    if (!wait_until([&]() {
            return yunlink_authority_current(air, &lease) == YUNLINK_RESULT_OK &&
                   lease.session_id == session.session_id && lease.lease_ttl_ms == 4500;
        })) {
        std::cerr << "authority renew not observed\n";
        return 11;
    }

    yunlink_goto_command_t goto_cmd{};
    goto_cmd.x_m = 5.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 3.0F;
    goto_cmd.yaw_rad = 0.25F;

    yunlink_command_handle_t command{};
    if (yunlink_command_publish_goto(ground, &air_peer, &session, &target, &goto_cmd, &command) !=
            YUNLINK_RESULT_OK ||
        command.message_id == 0 || command.session_id != session.session_id) {
        std::cerr << "publish goto failed\n";
        return 12;
    }

    yunlink_vehicle_core_state_t state{};
    state.armed = 1;
    state.nav_mode = 3;
    state.x_m = 11.0F;
    state.y_m = 12.0F;
    state.z_m = 13.0F;
    state.vx_mps = 0.1F;
    state.vy_mps = 0.2F;
    state.vz_mps = 0.3F;
    state.battery_percent = 76.5F;

    if (yunlink_publish_vehicle_core_state(
            air, &lease.peer, &ground_target, &state, session.session_id) != YUNLINK_RESULT_OK) {
        std::cerr << "publish vehicle core state failed\n";
        return 13;
    }

    yunlink_local_odom_t local_odom{};
    local_odom.source_stamp_ns = 1'234'567'890ULL;
    std::strncpy(local_odom.frame_id, "odom", sizeof(local_odom.frame_id) - 1);
    std::strncpy(local_odom.child_frame_id, "base_link", sizeof(local_odom.child_frame_id) - 1);
    local_odom.x_m = 21.0F;
    local_odom.y_m = -22.0F;
    local_odom.z_m = 23.0F;
    local_odom.orientation_z = 0.5F;
    local_odom.orientation_w = 0.8660254F;
    local_odom.vx_mps = 1.1F;
    local_odom.vy_mps = 1.2F;
    local_odom.vz_mps = 1.3F;
    local_odom.angular_z_radps = 0.4F;
    if (yunlink_publish_local_odom(
            air, &lease.peer, &ground_target, &local_odom, session.session_id) !=
        YUNLINK_RESULT_OK) {
        std::cerr << "publish local odom failed\n";
        return 21;
    }

    std::vector<uint8_t> phases;
    bool saw_state = false;
    bool saw_local_odom = false;
    if (!wait_until([&]() {
            yunlink_command_result_event_t result{};
            while (yunlink_runtime_poll_command_result(ground, &result) == YUNLINK_RESULT_OK) {
                phases.push_back(result.phase);
            }
            yunlink_vehicle_core_state_event_t state_event{};
            while (yunlink_runtime_poll_vehicle_core_state(ground, &state_event) ==
                   YUNLINK_RESULT_OK) {
                if (state_event.armed == 1 && state_event.battery_percent == 76.5F) {
                    saw_state = true;
                }
            }
            yunlink_runtime_event_t event{};
            while (yunlink_runtime_poll_event(ground, &event) == YUNLINK_RESULT_OK &&
                   event.type != YUNLINK_RUNTIME_EVENT_NONE) {
                if (event.type == YUNLINK_RUNTIME_EVENT_LOCAL_ODOM &&
                    event.data.local_odom.source_stamp_ns == local_odom.source_stamp_ns &&
                    event.data.local_odom.x_m == local_odom.x_m &&
                    event.data.local_odom.orientation_w == local_odom.orientation_w &&
                    event.data.local_odom.angular_z_radps == local_odom.angular_z_radps &&
                    std::strcmp(event.data.local_odom.frame_id, "odom") == 0) {
                    saw_local_odom = true;
                }
            }
            return saw_state && saw_local_odom && phases.size() >= 4;
        })) {
        std::cerr << "ffi event flow not observed\n";
        return 14;
    }
    const std::vector<uint8_t> expected_phases = {
        YUNLINK_COMMAND_PHASE_RECEIVED,
        YUNLINK_COMMAND_PHASE_ACCEPTED,
        YUNLINK_COMMAND_PHASE_IN_PROGRESS,
        YUNLINK_COMMAND_PHASE_SUCCEEDED,
    };
    if (std::vector<uint8_t>(phases.begin(), phases.begin() + 4) != expected_phases) {
        std::cerr << "ffi event order mismatch\n";
        return 15;
    }

    if (yunlink_authority_release(ground, &air_peer, &session, &target) != YUNLINK_RESULT_OK) {
        std::cerr << "authority release failed\n";
        return 16;
    }
    if (!wait_until(
            [&]() { return yunlink_authority_current(air, &lease) == YUNLINK_RESULT_NOT_FOUND; })) {
        std::cerr << "authority release not observed\n";
        return 17;
    }
    if (yunlink_authority_release(ground, &air_peer, &session, &target) != YUNLINK_RESULT_OK) {
        std::cerr << "repeated authority release failed\n";
        return 18;
    }

    if (yunlink_runtime_stop(ground) != YUNLINK_RESULT_OK ||
        yunlink_runtime_stop(ground) != YUNLINK_RESULT_OK ||
        yunlink_runtime_stop(air) != YUNLINK_RESULT_OK) {
        std::cerr << "runtime stop failed\n";
        return 19;
    }

    yunlink_runtime_destroy(ground);
    yunlink_runtime_destroy(air);
    return 0;
}
