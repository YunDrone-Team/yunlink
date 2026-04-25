/**
 * @file examples/cxx_air_roundtrip/main.cpp
 * @brief Minimal C++ air-side roundtrip helper for bindings integration.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool parse_u16_arg(const std::string& s, uint16_t* out) {
    char* end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (!end || *end != '\0' || v < 0 || v > 65535) {
        return false;
    }
    *out = static_cast<uint16_t>(v);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 4) {
        std::cerr << "usage: " << argv[0] << " <udp-bind> <udp-target> <tcp-listen>\n";
        return 2;
    }

    uint16_t udp_bind = 0;
    uint16_t udp_target = 0;
    uint16_t tcp_listen = 0;
    if (!parse_u16_arg(argv[1], &udp_bind) || !parse_u16_arg(argv[2], &udp_target) ||
        !parse_u16_arg(argv[3], &tcp_listen)) {
        std::cerr << "invalid ports\n";
        return 2;
    }

    yunlink::Runtime runtime;
    yunlink::RuntimeConfig cfg;
    cfg.udp_bind_port = udp_bind;
    cfg.udp_target_port = udp_target;
    cfg.tcp_listen_port = tcp_listen;
    cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    cfg.self_identity.agent_id = 1;
    cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    cfg.shared_secret = "yunlink-secret";

    if (runtime.start(cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    yunlink::AuthorityLease lease{};
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (std::chrono::steady_clock::now() < deadline) {
        if (runtime.current_authority(&lease)) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (lease.session_id == 0) {
        std::cerr << "authority lease not observed\n";
        runtime.stop();
        return 3;
    }

    yunlink::VehicleCoreState state{};
    state.armed = true;
    state.nav_mode = 3;
    state.x_m = 11.0F;
    state.y_m = 12.0F;
    state.z_m = 13.0F;
    state.vx_mps = 0.1F;
    state.vy_mps = 0.2F;
    state.vz_mps = 0.3F;
    state.battery_percent = 76.5F;

    if (runtime.publish_vehicle_core_state(lease.peer.id,
                                           yunlink::TargetSelector::broadcast(
                                               yunlink::AgentType::kGroundStation),
                                           state,
                                           lease.session_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "state publish failed\n";
        runtime.stop();
        return 4;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    runtime.stop();
    std::cout << "cxx-air-roundtrip ok\n";
    return 0;
}
