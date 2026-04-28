/**
 * @file examples/cxx_ground_peer_recovery/main.cpp
 * @brief Ground-side helper for explicit peer disconnect and recovery.
 */

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "yunlink/runtime/runtime.hpp"

namespace {

using Clock = std::chrono::steady_clock;

bool parse_u16_arg(const std::string& s, uint16_t* out) {
    char* end = nullptr;
    const long value = std::strtol(s.c_str(), &end, 10);
    if (!end || *end != '\0' || value < 0 || value > 65535) {
        return false;
    }
    *out = static_cast<uint16_t>(value);
    return true;
}

double elapsed_ms(Clock::time_point start) {
    return std::chrono::duration<double, std::milli>(Clock::now() - start).count();
}

bool wait_until(const std::function<bool()>& pred, int retries = 200, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

struct Observed {
    uint64_t session_id = 0;
    uint64_t state_message_id = 0;
    int result_count = 0;
};

bool roundtrip_complete(const std::vector<Observed>& observed, uint64_t session_id) {
    for (const auto& item : observed) {
        if (item.session_id == session_id && item.state_message_id != 0 && item.result_count >= 4) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 6) {
        std::cerr << "usage: " << argv[0] << " <ip> <port> <udp-bind> <udp-target> <tcp-listen>\n";
        return 2;
    }

    uint16_t port = 0;
    uint16_t udp_bind = 0;
    uint16_t udp_target = 0;
    uint16_t tcp_listen = 0;
    if (!parse_u16_arg(argv[2], &port) || !parse_u16_arg(argv[3], &udp_bind) ||
        !parse_u16_arg(argv[4], &udp_target) || !parse_u16_arg(argv[5], &tcp_listen)) {
        std::cerr << "invalid ports\n";
        return 2;
    }

    yunlink::Runtime runtime;
    yunlink::RuntimeConfig cfg;
    cfg.udp_bind_port = udp_bind;
    cfg.udp_target_port = udp_target;
    cfg.tcp_listen_port = tcp_listen;
    cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    cfg.self_identity.agent_id = 7;
    cfg.self_identity.role = yunlink::EndpointRole::kController;
    cfg.shared_secret = "yunlink-secret";

    if (runtime.start(cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "failed to start runtime\n";
        return 1;
    }

    std::mutex mu;
    std::vector<Observed> observed;
    const size_t result_token = runtime.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            for (auto& item : observed) {
                if (item.session_id == view.envelope.session_id) {
                    ++item.result_count;
                    return;
                }
            }
            Observed item{};
            item.session_id = view.envelope.session_id;
            item.result_count = 1;
            observed.push_back(item);
        });
    const size_t state_token = runtime.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            if ((message.payload.battery_percent - 76.5F) > 0.001F ||
                (message.payload.battery_percent - 76.5F) < -0.001F) {
                return;
            }
            std::lock_guard<std::mutex> lock(mu);
            for (auto& item : observed) {
                if (item.session_id == message.envelope.session_id) {
                    item.state_message_id = message.envelope.message_id;
                    return;
                }
            }
            Observed item{};
            item.session_id = message.envelope.session_id;
            item.state_message_id = message.envelope.message_id;
            observed.push_back(item);
        });

    const std::string ip = argv[1];
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 5.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 3.0F;
    goto_cmd.yaw_rad = 0.25F;

    const auto connect_started = Clock::now();
    std::string peer_id;
    if (runtime.tcp_clients().connect_peer(ip, port, &peer_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 3;
    }
    const double connect_ms = elapsed_ms(connect_started);

    const auto session_started = Clock::now();
    const uint64_t session1 = runtime.session_client().open_active_session(peer_id, "cxx-ground-1");
    yunlink::SessionDescriptor session_desc{};
    if (session1 == 0 || !wait_until([&]() {
            return runtime.session_server().describe_session(peer_id, session1, &session_desc) &&
                   session_desc.state == yunlink::SessionState::kActive;
        })) {
        std::cerr << "session1 did not become active\n";
        return 4;
    }
    const double session_ready_ms = elapsed_ms(session_started);

    const auto authority_started = Clock::now();
    if (runtime.request_authority(
            peer_id, session1, target, yunlink::ControlSource::kGroundStation, 3000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 5;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& item : observed) {
                if (item.session_id == session1 && item.state_message_id != 0) {
                    return true;
                }
            }
            return false;
        })) {
        std::cerr << "state not observed for session1\n";
        return 6;
    }
    const double state_first_seen_ms = elapsed_ms(authority_started);

    const auto command_started = Clock::now();
    if (runtime.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish goto for session1 failed\n";
        return 7;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return roundtrip_complete(observed, session1);
        })) {
        std::cerr << "session1 roundtrip incomplete\n";
        return 8;
    }
    const double command_result_ms = elapsed_ms(command_started);

    const auto recovery_started = Clock::now();
    runtime.tcp_clients().close_peer(peer_id);
    if (!wait_until([&]() {
            return runtime.session_server().describe_session(peer_id, session1, &session_desc) &&
                   session_desc.state == yunlink::SessionState::kLost;
        })) {
        std::cerr << "session1 did not become lost after close_peer\n";
        return 9;
    }

    if (runtime.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kRejected) {
        std::cerr << "old lost session did not reject publish\n";
        return 10;
    }

    if (runtime.tcp_clients().connect_peer(ip, port, &peer_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "reconnect failed\n";
        return 11;
    }
    const uint64_t session2 = runtime.session_client().open_active_session(peer_id, "cxx-ground-2");
    if (session2 == 0 || !wait_until([&]() {
            return runtime.session_server().describe_session(peer_id, session2, &session_desc) &&
                   session_desc.state == yunlink::SessionState::kActive;
        })) {
        std::cerr << "session2 did not become active\n";
        return 12;
    }

    if (runtime.request_authority(
            peer_id, session2, target, yunlink::ControlSource::kGroundStation, 3000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority reacquire failed\n";
        return 13;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& item : observed) {
                if (item.session_id == session2 && item.state_message_id != 0) {
                    return true;
                }
            }
            return false;
        })) {
        std::cerr << "state not observed for session2\n";
        return 14;
    }

    if (runtime.command_publisher().publish_goto(peer_id, session2, target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish goto for session2 failed\n";
        return 15;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return roundtrip_complete(observed, session2);
        })) {
        std::cerr << "session2 roundtrip incomplete\n";
        return 16;
    }
    const double recovery_ms = elapsed_ms(recovery_started);

    if (runtime.release_authority(peer_id, session2, target) != yunlink::ErrorCode::kOk) {
        std::cerr << "release authority failed\n";
        return 17;
    }

    runtime.state_subscriber().unsubscribe(state_token);
    runtime.event_subscriber().unsubscribe(result_token);
    runtime.stop();
    std::cout << "YUNLINK_METRICS "
              << "{\"connect_ms\":" << connect_ms << ",\"session_ready_ms\":" << session_ready_ms
              << ",\"authority_acquire_ms\":" << state_first_seen_ms
              << ",\"command_result_ms\":" << command_result_ms
              << ",\"state_first_seen_ms\":" << state_first_seen_ms
              << ",\"recovery_ms\":" << recovery_ms << "}\n";
    std::cout << "cxx-ground-peer-recovery ok\n";
    return 0;
}
