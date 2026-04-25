/**
 * @file tests/runtime/test_authority_status.cpp
 * @brief AuthorityStatus active reply tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

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

bool has_status(const std::vector<yunlink::TypedMessage<yunlink::AuthorityStatus>>& statuses,
                yunlink::AuthorityState state,
                uint64_t session_id,
                const std::string& detail) {
    for (const auto& status : statuses) {
        if (status.payload.state == state && status.payload.session_id == session_id &&
            status.payload.detail == detail) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client_a;
    yunlink::Runtime client_b;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12530;
    server_cfg.udp_target_port = 12530;
    server_cfg.tcp_listen_port = 12630;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "authority-status-secret";

    yunlink::RuntimeConfig client_a_cfg;
    client_a_cfg.udp_bind_port = 12531;
    client_a_cfg.udp_target_port = 12531;
    client_a_cfg.tcp_listen_port = 12631;
    client_a_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_a_cfg.self_identity.agent_id = 7;
    client_a_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_a_cfg.shared_secret = "authority-status-secret";

    yunlink::RuntimeConfig client_b_cfg = client_a_cfg;
    client_b_cfg.udp_bind_port = 12532;
    client_b_cfg.udp_target_port = 12532;
    client_b_cfg.tcp_listen_port = 12632;
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

    std::mutex mu;
    std::vector<yunlink::TypedMessage<yunlink::AuthorityStatus>> statuses_a;
    std::vector<yunlink::TypedMessage<yunlink::AuthorityStatus>> statuses_b;
    const size_t token_a = client_a.event_subscriber().subscribe_authority_status(
        [&](const yunlink::TypedMessage<yunlink::AuthorityStatus>& status) {
            std::lock_guard<std::mutex> lock(mu);
            statuses_a.push_back(status);
        });
    const size_t token_b = client_b.event_subscriber().subscribe_authority_status(
        [&](const yunlink::TypedMessage<yunlink::AuthorityStatus>& status) {
            std::lock_guard<std::mutex> lock(mu);
            statuses_b.push_back(status);
        });

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (client_a.request_authority(
            peer_a, session_a, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "client_a claim failed\n";
        return 4;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_status(statuses_a,
                              yunlink::AuthorityState::kController,
                              session_a,
                              "authority-granted");
        })) {
        std::cerr << "client_a did not receive granted status\n";
        return 5;
    }

    if (client_b.request_authority(
            peer_b, session_b, target, yunlink::ControlSource::kGroundStation, 2000, false) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "client_b non-preempt claim failed\n";
        return 6;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_status(
                statuses_b, yunlink::AuthorityState::kRejected, session_b, "authority-held");
        })) {
        std::cerr << "client_b did not receive rejected status\n";
        return 7;
    }

    if (client_b.request_authority(
            peer_b, session_b, target, yunlink::ControlSource::kGroundStation, 2000, true) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "client_b preempt claim failed\n";
        return 8;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_status(statuses_a,
                              yunlink::AuthorityState::kRevoked,
                              session_a,
                              "authority-preempted") &&
                   has_status(statuses_b,
                              yunlink::AuthorityState::kController,
                              session_b,
                              "authority-granted");
        })) {
        std::cerr << "preempt statuses missing\n";
        return 9;
    }

    if (client_b.release_authority(peer_b, session_b, target) != yunlink::ErrorCode::kOk) {
        std::cerr << "client_b release failed\n";
        return 10;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_status(
                statuses_b, yunlink::AuthorityState::kReleased, session_b, "authority-released");
        })) {
        std::cerr << "client_b did not receive released status\n";
        return 11;
    }

    client_a.event_subscriber().unsubscribe(token_a);
    client_b.event_subscriber().unsubscribe(token_b);
    client_b.stop();
    client_a.stop();
    server.stop();
    return 0;
}
