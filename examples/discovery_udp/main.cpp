/**
 * @file examples/discovery_udp/main.cpp
 * @brief yunlink source file.
 */

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <thread>

#include "yunlink/core/semantic_messages.hpp"
#include "yunlink/runtime/runtime.hpp"

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

    yunlink::Runtime runtime;
    yunlink::RuntimeConfig cfg;
    cfg.udp_bind_port = udp_bind;
    cfg.udp_target_port = udp_target;

    if (runtime.start(cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    auto tok = runtime.event_bus().subscribe_envelope([](const yunlink::EnvelopeEvent& ev) {
        std::cout << "rx family=" << static_cast<int>(ev.envelope.message_family) << " from "
                  << ev.peer.ip << ":" << ev.peer.port << "\n";
    });

    yunlink::ProtocolCodec codec;
    yunlink::VehicleEvent discovery{};
    discovery.kind = yunlink::VehicleEventKind::kInfo;
    discovery.severity = 1;
    discovery.detail = "discovery";
    auto bytes = codec.encode(
        yunlink::make_typed_envelope(
            {yunlink::AgentType::kGroundStation, 0, yunlink::EndpointRole::kObserver},
            yunlink::TargetSelector::broadcast(yunlink::AgentType::kUnknown),
            0,
            0,
            yunlink::QosClass::kBestEffort,
            discovery),
        true);
    runtime.udp().send_broadcast(bytes, cfg.udp_target_port);

    std::this_thread::sleep_for(std::chrono::milliseconds(hold_ms));
    runtime.event_bus().unsubscribe(tok);
    runtime.stop();
    return 0;
}
