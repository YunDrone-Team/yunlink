/**
 * @file tests/runtime/test_authority_target_partition.cpp
 * @brief Target-scoped authority lease partition tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 120, int sleep_ms = 20) {
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
    yunlink::Runtime server;
    yunlink::Runtime client_a;
    yunlink::Runtime client_b;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12520;
    server_cfg.udp_target_port = 12520;
    server_cfg.tcp_listen_port = 12620;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "authority-partition-secret";

    yunlink::RuntimeConfig client_a_cfg;
    client_a_cfg.udp_bind_port = 12521;
    client_a_cfg.udp_target_port = 12521;
    client_a_cfg.tcp_listen_port = 12621;
    client_a_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_a_cfg.self_identity.agent_id = 7;
    client_a_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_a_cfg.shared_secret = "authority-partition-secret";

    yunlink::RuntimeConfig client_b_cfg = client_a_cfg;
    client_b_cfg.udp_bind_port = 12522;
    client_b_cfg.udp_target_port = 12522;
    client_b_cfg.tcp_listen_port = 12622;
    client_b_cfg.self_identity.agent_id = 8;

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client_a.start(client_a_cfg) != yunlink::ErrorCode::kOk ||
        client_b.start(client_b_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_a;
    std::string peer_b;
    if (client_a.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_a) !=
            yunlink::ErrorCode::kOk ||
        client_b.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_b) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_a = client_a.session_client().open_active_session(peer_a, "station-a");
    const uint64_t session_b = client_b.session_client().open_active_session(peer_b, "station-b");
    if (session_a == 0 || session_b == 0 ||
        !wait_until([&]() {
            return server.session_server().has_active_session(session_a) &&
                   server.session_server().has_active_session(session_b);
        })) {
        std::cerr << "sessions not active\n";
        return 3;
    }

    const auto target_a = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    const auto target_b = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 2);

    if (client_a.request_authority(
            peer_a, session_a, target_a, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk ||
        client_b.request_authority(
            peer_b, session_b, target_b, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "authority requests failed\n";
        return 4;
    }

    yunlink::AuthorityLease lease_a{};
    yunlink::AuthorityLease lease_b{};
    if (!wait_until([&]() {
            return server.current_authority_for_target(target_a, &lease_a) &&
                   server.current_authority_for_target(target_b, &lease_b) &&
                   lease_a.session_id == session_a && lease_b.session_id == session_b;
        })) {
        std::cerr << "target-scoped authority leases were not independent\n";
        return 5;
    }

    if (client_a.release_authority(peer_a, session_a, target_a) != yunlink::ErrorCode::kOk) {
        std::cerr << "release target_a failed\n";
        return 6;
    }
    if (!wait_until([&]() {
            return !server.current_authority_for_target(target_a, nullptr) &&
                   server.current_authority_for_target(target_b, &lease_b) &&
                   lease_b.session_id == session_b;
        })) {
        std::cerr << "target release leaked across authority partition\n";
        return 7;
    }

    client_b.stop();
    client_a.stop();
    server.stop();
    return 0;
}
