/**
 * @file tests/test_compat_roundtrip.cpp
 * @brief sunray_communication_lib source file.
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/runtime/runtime.hpp"

int main() {
    sunraycom::Runtime server;
    sunraycom::Runtime client;

    sunraycom::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12310;
    server_cfg.udp_target_port = 12310;
    server_cfg.tcp_listen_port = 12410;
    server_cfg.self_identity.agent_type = sunraycom::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = sunraycom::EndpointRole::kVehicle;
    server_cfg.shared_secret = "sunray-secret";

    sunraycom::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12311;
    client_cfg.udp_target_port = 12311;
    client_cfg.tcp_listen_port = 12411;
    client_cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 7;
    client_cfg.self_identity.role = sunraycom::EndpointRole::kController;
    client_cfg.shared_secret = "sunray-secret";

    if (server.start(server_cfg) != sunraycom::ErrorCode::kOk ||
        client.start(client_cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_id) !=
        sunraycom::ErrorCode::kOk) {
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

    sunraycom::SessionDescriptor desc{};
    if (!server.session_server().describe_session(session_id, &desc) ||
        desc.state != sunraycom::SessionState::kActive) {
        std::cerr << "server session did not become active\n";
        return 4;
    }

    const auto target = sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kUav, 1);
    if (client.request_authority(
            peer_id, session_id, target, sunraycom::ControlSource::kGroundStation, 2000) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "authority request send failed\n";
        return 5;
    }

    sunraycom::AuthorityLease lease{};
    for (int i = 0; i < 100; ++i) {
        if (server.current_authority(&lease) &&
            lease.state == sunraycom::AuthorityState::kController &&
            lease.session_id == session_id) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    if (lease.state != sunraycom::AuthorityState::kController || lease.session_id != session_id) {
        std::cerr << "authority lease not granted\n";
        return 6;
    }

    std::vector<sunraycom::CommandPhase> phases;
    const size_t tok = client.event_subscriber().subscribe_command_results(
        [&phases](const sunraycom::CommandResultView& view) {
            phases.push_back(view.payload.phase);
        });

    sunraycom::GotoCommand goto_cmd{};
    goto_cmd.x_m = 10.0F;
    goto_cmd.y_m = 2.0F;
    goto_cmd.z_m = 4.0F;
    goto_cmd.yaw_rad = 0.5F;

    sunraycom::CommandHandle handle{};
    if (client.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "publish goto failed\n";
        return 7;
    }

    for (int i = 0; i < 100 && phases.size() < 4; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    client.event_subscriber().unsubscribe(tok);
    client.stop();
    server.stop();

    if (phases.size() < 4 || phases[0] != sunraycom::CommandPhase::kReceived ||
        phases[1] != sunraycom::CommandPhase::kAccepted ||
        phases[2] != sunraycom::CommandPhase::kInProgress ||
        phases[3] != sunraycom::CommandPhase::kSucceeded) {
        std::cerr << "command result flow mismatch\n";
        return 8;
    }

    return 0;
}
