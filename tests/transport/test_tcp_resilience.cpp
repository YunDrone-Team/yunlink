/**
 * @file tests/transport/test_tcp_resilience.cpp
 * @brief TCP connect / duplicate connect / bind failure regressions.
 */

#include <iostream>

#include "yunlink/runtime/runtime.hpp"

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client;
    yunlink::Runtime conflicting;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12370;
    server_cfg.udp_target_port = 12370;
    server_cfg.tcp_listen_port = 12470;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12371;
    client_cfg.udp_target_port = 12371;
    client_cfg.tcp_listen_port = 12471;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 7;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;

    yunlink::RuntimeConfig conflict_cfg = client_cfg;
    conflict_cfg.udp_bind_port = 12372;
    conflict_cfg.udp_target_port = 12372;
    conflict_cfg.tcp_listen_port = server_cfg.tcp_listen_port;
    conflict_cfg.self_identity.agent_id = 8;

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_a;
    std::string peer_b;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_a) !=
            yunlink::ErrorCode::kOk ||
        client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_b) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "duplicate connect failed\n";
        return 2;
    }
    if (peer_a.empty() || peer_a != peer_b) {
        std::cerr << "duplicate connect did not resolve to the same peer id\n";
        return 3;
    }

    std::string missing_peer;
    const auto missing_ec =
        client.tcp_clients().connect_peer("127.0.0.1", 6550, &missing_peer);
    if (missing_ec != yunlink::ErrorCode::kConnectError &&
        missing_ec != yunlink::ErrorCode::kTimeout) {
        std::cerr << "unexpected error for unavailable listener\n";
        return 4;
    }

    if (conflicting.start(conflict_cfg) != yunlink::ErrorCode::kBindError) {
        std::cerr << "conflicting tcp listener did not return bind error\n";
        return 5;
    }

    client.stop();
    server.stop();
    return 0;
}
