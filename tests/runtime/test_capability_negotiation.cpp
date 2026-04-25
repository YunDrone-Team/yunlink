/**
 * @file tests/runtime/test_capability_negotiation.cpp
 * @brief Runtime capability negotiation rejection tests.
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
    yunlink::Runtime client;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12510;
    server_cfg.udp_target_port = 12510;
    server_cfg.tcp_listen_port = 12610;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "capability-secret";
    server_cfg.capability_flags = 0x4;

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12511;
    client_cfg.udp_target_port = 12511;
    client_cfg.tcp_listen_port = 12611;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 7;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_cfg.shared_secret = "capability-secret";
    client_cfg.capability_flags = 0x0;

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_id = client.session_client().open_active_session(peer_id, "no-bulk");
    if (session_id == 0) {
        std::cerr << "session open failed\n";
        return 3;
    }

    yunlink::SessionDescriptor desc{};
    if (!wait_until([&]() {
            return server.session_server().describe_session(session_id, &desc) &&
                   desc.state == yunlink::SessionState::kInvalid;
        })) {
        std::cerr << "capability mismatch did not invalidate session\n";
        return 4;
    }
    if (server.session_server().has_active_session(session_id)) {
        std::cerr << "capability mismatch session became active\n";
        return 5;
    }

    client.stop();
    server.stop();
    return 0;
}
