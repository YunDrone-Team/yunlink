/**
 * @file tests/test_compat_roundtrip.cpp
 * @brief yunlink source file.
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "yunlink/core/semantic_messages.hpp"
#include "yunlink/runtime/runtime.hpp"

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12310;
    server_cfg.udp_target_port = 12310;
    server_cfg.tcp_listen_port = 12410;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12311;
    client_cfg.udp_target_port = 12311;
    client_cfg.tcp_listen_port = 12411;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 7;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_cfg.shared_secret = "yunlink-secret";

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_id = client.session_client().open_active_session(peer_id, "station");
    if (session_id == 0) {
        std::cerr << "session open failed\n";
        return 3;
    }

    for (int i = 0; i < 100 && !server.session_server().has_active_session(session_id); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    yunlink::SessionDescriptor desc{};
    if (!server.session_server().describe_session(session_id, &desc) ||
        desc.state != yunlink::SessionState::kActive) {
        std::cerr << "server session did not become active\n";
        return 4;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (client.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority request send failed\n";
        return 5;
    }

    yunlink::AuthorityLease lease{};
    for (int i = 0; i < 100; ++i) {
        if (server.current_authority(&lease) &&
            lease.state == yunlink::AuthorityState::kController && lease.session_id == session_id) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (lease.state != yunlink::AuthorityState::kController || lease.session_id != session_id) {
        std::cerr << "authority lease not granted\n";
        return 6;
    }

    std::vector<yunlink::CommandPhase> phases;
    const size_t tok = client.event_subscriber().subscribe_command_results(
        [&phases](const yunlink::CommandResultView& view) {
            phases.push_back(view.payload.phase);
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 10.0F;
    goto_cmd.y_m = 2.0F;
    goto_cmd.z_m = 4.0F;
    goto_cmd.yaw_rad = 0.5F;

    yunlink::CommandHandle handle{};
    if (client.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish goto failed\n";
        return 7;
    }

    for (int i = 0; i < 100 && phases.size() < 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    client.event_subscriber().unsubscribe(tok);
    client.stop();
    server.stop();

    if (phases.size() < 4 || phases[0] != yunlink::CommandPhase::kReceived ||
        phases[1] != yunlink::CommandPhase::kAccepted ||
        phases[2] != yunlink::CommandPhase::kInProgress ||
        phases[3] != yunlink::CommandPhase::kSucceeded) {
        std::cerr << "command result flow mismatch\n";
        return 8;
    }

    return 0;
}
