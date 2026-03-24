/**
 * @file examples/udp_tcp_bridge/main.cpp
 * @brief SunrayComLib source file.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

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

bool parse_i32_arg(const std::string& s, int* out) {
    char* end = nullptr;
    const long v = std::strtol(s.c_str(), &end, 10);
    if (!end || *end != '\0')
        return false;
    if (v < -2147483647L - 1L || v > 2147483647L)
        return false;
    *out = static_cast<int>(v);
    return true;
}

void print_usage(const char* prog) {
    std::cerr
        << "usage: " << prog
        << " <tcp_ip> <tcp_port> [--udp-bind <port>] [--tcp-listen <port>] [--duration-ms <n>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const std::string tcp_ip = argv[1];
    uint16_t tcp_port = 0;
    if (!parse_u16_arg(argv[2], &tcp_port)) {
        print_usage(argv[0]);
        return 1;
    }

    uint16_t udp_bind = 9696;
    uint16_t tcp_listen = 9696;
    int duration_ms = 20000;

    for (int i = 3; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--udp-bind" && i + 1 < argc) {
            if (!parse_u16_arg(argv[++i], &udp_bind)) {
                print_usage(argv[0]);
                return 2;
            }
        } else if (arg == "--tcp-listen" && i + 1 < argc) {
            if (!parse_u16_arg(argv[++i], &tcp_listen)) {
                print_usage(argv[0]);
                return 2;
            }
        } else if (arg == "--duration-ms" && i + 1 < argc) {
            if (!parse_i32_arg(argv[++i], &duration_ms) || duration_ms <= 0) {
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
    cfg.tcp_listen_port = tcp_listen;
    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    std::string peer_id;
    if (runtime.tcp_clients().connect_peer(tcp_ip, tcp_port, &peer_id) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        runtime.stop();
        return 2;
    }

    auto tok =
        runtime.event_bus().subscribe_frame([&runtime, &peer_id](const sunraycom::FrameEvent& ev) {
            if (ev.transport == sunraycom::TransportType::kUdpUnicast ||
                ev.transport == sunraycom::TransportType::kUdpBroadcast) {
                sunraycom::ProtocolCodec codec;
                const auto bytes = codec.encode(ev.frame, false);
                runtime.tcp_clients().send(peer_id, bytes);
            }
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(duration_ms));
    runtime.event_bus().unsubscribe(tok);
    runtime.stop();
    return 0;
}
