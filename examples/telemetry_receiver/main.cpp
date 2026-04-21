/**
 * @file examples/telemetry_receiver/main.cpp
 * @brief sunray_communication_lib source file.
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
    std::cerr << "usage: " << prog
              << " [--udp-bind <port>] [--tcp-listen <port>] [--timeout-ms <n>] [--required-frames "
                 "<n>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    uint16_t udp_bind = 9696;
    uint16_t tcp_listen = 9696;
    int timeout_ms = 10000;
    int required_frames = 0;

    for (int i = 1; i < argc; ++i) {
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
        } else if (arg == "--timeout-ms" && i + 1 < argc) {
            if (!parse_i32_arg(argv[++i], &timeout_ms) || timeout_ms <= 0) {
                print_usage(argv[0]);
                return 2;
            }
        } else if (arg == "--required-frames" && i + 1 < argc) {
            if (!parse_i32_arg(argv[++i], &required_frames) || required_frames < 0) {
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

    int got_frames = 0;
    auto tok =
        runtime.event_bus().subscribe_envelope([&got_frames](const sunraycom::EnvelopeEvent& ev) {
            ++got_frames;
            std::cout << "transport=" << static_cast<int>(ev.transport)
                      << " family=" << static_cast<int>(ev.envelope.message_family)
                      << " type=" << ev.envelope.message_type
                      << " payload=" << ev.envelope.payload.size() << " peer=" << ev.peer.id
                      << "\n";
        });

    const auto start = std::chrono::steady_clock::now();
    while (true) {
        if (required_frames > 0 && got_frames >= required_frames) {
            break;
        }
        const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start);
        if (elapsed.count() >= timeout_ms) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    runtime.event_bus().unsubscribe(tok);
    runtime.stop();

    if (required_frames > 0 && got_frames < required_frames) {
        std::cerr << "required frames not reached: got=" << got_frames
                  << ", required=" << required_frames << "\n";
        return 3;
    }

    return 0;
}
