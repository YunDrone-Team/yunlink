/**
 * @file examples/tcp_command_client/main.cpp
 * @brief SunrayComLib source file.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "sunraycom/compat/legacy_adapter.hpp"
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

    sunraycom::compat::LegacyAdapter adapter;
    ::DataFrame hb{};
    hb.seq = MessageID::HeartbeatMessageID;
    hb.robot_ID = 1;
    hb.data.heartbeat.init();
    hb.data.heartbeat.agentType = 1;
    hb.data.heartbeat.count = 1;

    const auto bytes = adapter.encode_legacy(hb);
    const int sent = runtime.tcp_clients().send(peer_id, bytes);
    std::cout << "sent=" << sent << " bytes to " << peer_id << "\n";

    runtime.tcp_clients().close_peer(peer_id);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    runtime.stop();
    return 0;
}
