/**
 * @file examples/tcp_command_client/main.cpp
 * @brief sunray_communication_lib source file.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/runtime/runtime.hpp"

namespace {

bool parse_u16_arg(const std::string& s, uint16_t* out) {
    char* end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (!end || *end != '\0')
        return false;
    if (v < 0 || v > 65535)
        return false;
    *out = static_cast<uint16_t>(v);
    return true;
}

void print_usage(const char* prog) {
    std::cerr << "usage: " << prog
              << " <ip> <port> [--udp-bind <port>] [--udp-target <port>] [--tcp-listen <port>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string ip = argv[1];
    uint16_t port = 0;
    if (!parse_u16_arg(argv[2], &port)) {
        print_usage(argv[0]);
        return 1;
    }

    uint16_t udp_bind = 9696;
    uint16_t udp_target = 9898;
    uint16_t tcp_listen = 9696;

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--udp-bind" && i + 1 < argc) {
            if (!parse_u16_arg(argv[++i], &udp_bind)) {
                print_usage(argv[0]);
                return 2;
            }
        } else if (arg == "--udp-target" && i + 1 < argc) {
            if (!parse_u16_arg(argv[++i], &udp_target)) {
                print_usage(argv[0]);
                return 2;
            }
        } else if (arg == "--tcp-listen" && i + 1 < argc) {
            if (!parse_u16_arg(argv[++i], &tcp_listen)) {
                print_usage(argv[0]);
                return 2;
            }
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    sunraycom::Runtime runtime;
    sunraycom::RuntimeConfig cfg;
    cfg.udp_bind_port = udp_bind;
    cfg.udp_target_port = udp_target;
    cfg.tcp_listen_port = tcp_listen;
    cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    cfg.self_identity.agent_id = 7;
    cfg.self_identity.role = sunraycom::EndpointRole::kController;
    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    std::string peer_id;
    if (runtime.tcp_clients().connect_peer(ip, port, &peer_id) != sunraycom::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        runtime.stop();
        return 2;
    }

    const uint64_t session_id = runtime.session_client().open_active_session(peer_id, "tcp_client");
    if (session_id == 0) {
        std::cerr << "open session failed\n";
        runtime.stop();
        return 3;
    }

    const auto target = sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kUav, 1);
    runtime.request_authority(
        peer_id, session_id, target, sunraycom::ControlSource::kGroundStation, 3000);

    sunraycom::GotoCommand cmd{};
    cmd.x_m = 5.0F;
    cmd.y_m = 0.0F;
    cmd.z_m = 3.0F;
    cmd.yaw_rad = 0.2F;
    sunraycom::CommandHandle handle{};
    const auto ec =
        runtime.command_publisher().publish_goto(peer_id, session_id, target, cmd, &handle);
    std::cout << "session=" << session_id << " message=" << handle.message_id
              << " status=" << static_cast<int>(ec) << " peer=" << peer_id << "\n";

    runtime.tcp_clients().close_peer(peer_id);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    runtime.stop();
    return 0;
}
