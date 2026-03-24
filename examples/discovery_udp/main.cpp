/**
 * @file examples/discovery_udp/main.cpp
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
              << " [--udp-bind <port>] [--udp-target <port>] [--hold-ms <n>]\n";
}

}  // namespace

int main(int argc, char** argv) {
    uint16_t udp_bind = 9696;
    uint16_t udp_target = 9898;
    int hold_ms = 3000;

    for (int i = 1; i < argc; ++i) {
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
        } else if (arg == "--hold-ms" && i + 1 < argc) {
            if (!parse_i32_arg(argv[++i], &hold_ms) || hold_ms <= 0) {
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

    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    auto tok = runtime.event_bus().subscribe_frame([](const sunraycom::FrameEvent& ev) {
        std::cout << "rx seq=" << static_cast<int>(ev.frame.header.seq) << " from " << ev.peer.ip
                  << ":" << ev.peer.port << "\n";
    });

    sunraycom::compat::LegacyAdapter adapter;
    ::DataFrame df{};
    df.data.search.init();
    df.data.search.port = cfg.udp_bind_port;
    df.seq = MessageID::SearchMessageID;
    df.robot_ID = 0;

    auto bytes = adapter.encode_legacy(df);
    runtime.udp().send_broadcast(bytes, cfg.udp_target_port);

    std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    runtime.event_bus().unsubscribe(tok);
    runtime.stop();
    return 0;
}
