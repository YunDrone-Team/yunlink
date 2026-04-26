/**
 * @file tests/runtime/test_session_lifecycle.cpp
 * @brief Bidirectional session activation and loss convergence tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 160, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12500;
    air_cfg.udp_target_port = 12500;
    air_cfg.tcp_listen_port = 12600;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "session-lifecycle-secret";
    air_cfg.capability_flags = 0x1;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12501;
    ground_cfg.udp_target_port = 12501;
    ground_cfg.tcp_listen_port = 12601;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 7;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "session-lifecycle-secret";
    ground_cfg.capability_flags = 0x1;

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ground");
    if (session_id == 0) {
        std::cerr << "session open failed\n";
        return 3;
    }

    yunlink::SessionDescriptor server_session{};
    if (!wait_until([&]() {
            return air.session_server().describe_session(session_id, &server_session) &&
                   server_session.state == yunlink::SessionState::kActive;
        })) {
        std::cerr << "server session did not become active\n";
        return 4;
    }
    const std::string server_peer_id = server_session.peer.id;

    yunlink::SessionDescriptor client_session{};
    if (!wait_until([&]() {
            return ground.session_server().describe_session(peer_id, session_id, &client_session) &&
                   client_session.state == yunlink::SessionState::kActive;
        })) {
        std::cerr << "client session did not observe remote ready ack\n";
        return 5;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 6;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until(
            [&]() { return air.current_authority(&lease) && lease.session_id == session_id; })) {
        std::cerr << "authority not granted\n";
        return 7;
    }

    ground.stop();

    if (!wait_until([&]() {
            return air.session_server().describe_session(
                       server_peer_id, session_id, &server_session) &&
                   server_session.state == yunlink::SessionState::kLost;
        })) {
        std::cerr << "server session did not converge to lost on link down\n";
        return 8;
    }
    if (air.current_authority(&lease)) {
        std::cerr << "authority survived session loss\n";
        return 9;
    }

    air.stop();
    return 0;
}
