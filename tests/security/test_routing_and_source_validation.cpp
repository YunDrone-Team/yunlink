/**
 * @file tests/security/test_routing_and_source_validation.cpp
 * @brief target routing isolation and invalid authority source validation.
 */

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

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
    yunlink::Runtime air1;
    yunlink::Runtime air2;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air1_cfg;
    air1_cfg.udp_bind_port = 12380;
    air1_cfg.udp_target_port = 12380;
    air1_cfg.tcp_listen_port = 12480;
    air1_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air1_cfg.self_identity.agent_id = 1;
    air1_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air1_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig air2_cfg = air1_cfg;
    air2_cfg.udp_bind_port = 12381;
    air2_cfg.udp_target_port = 12381;
    air2_cfg.tcp_listen_port = 12481;
    air2_cfg.self_identity.agent_id = 2;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12382;
    ground_cfg.udp_target_port = 12382;
    ground_cfg.tcp_listen_port = 12482;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 7;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "yunlink-secret";

    if (air1.start(air1_cfg) != yunlink::ErrorCode::kOk ||
        air2.start(air2_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer1;
    std::string peer2;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air1_cfg.tcp_listen_port, &peer1) !=
            yunlink::ErrorCode::kOk ||
        ground.tcp_clients().connect_peer("127.0.0.1", air2_cfg.tcp_listen_port, &peer2) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session1 = ground.session_client().open_active_session(peer1, "ground-air1");
    const uint64_t session2 = ground.session_client().open_active_session(peer2, "ground-air2");
    if (session1 == 0 || session2 == 0 ||
        !wait_until([&]() {
            return air1.session_server().has_active_session(session1) &&
                   air2.session_server().has_active_session(session2);
        })) {
        std::cerr << "sessions not active\n";
        return 3;
    }

    yunlink::SessionDescriptor air1_session{};
    yunlink::SessionDescriptor air2_session{};
    if (!air1.session_server().describe_session(session1, &air1_session) ||
        air1_session.peer.id.empty() ||
        !air2.session_server().describe_session(session2, &air2_session) ||
        air2_session.peer.id.empty()) {
        std::cerr << "session peer not resolved\n";
        return 4;
    }

    if (ground.request_authority(peer1,
                                 session1,
                                 yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
                                 yunlink::ControlSource::kUnknown,
                                 1000) != yunlink::ErrorCode::kInvalidArgument) {
        std::cerr << "invalid authority source was not rejected\n";
        return 5;
    }

    if (ground.request_authority(peer1,
                                 session1,
                                 yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
                                 yunlink::ControlSource::kGroundStation,
                                 1000) != yunlink::ErrorCode::kOk ||
        ground.request_authority(peer2,
                                 session2,
                                 yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 2),
                                 yunlink::ControlSource::kGroundStation,
                                 1000) != yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 6;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air1.current_authority(&lease) && lease.session_id == session1; }) ||
        !wait_until([&]() { return air2.current_authority(&lease) && lease.session_id == session2; })) {
        std::cerr << "authority not granted\n";
        return 7;
    }

    std::vector<yunlink::CommandPhase> session1_results;
    std::vector<yunlink::CommandPhase> session2_results;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            if (view.envelope.session_id == session1) {
                session1_results.push_back(view.payload.phase);
            }
            if (view.envelope.session_id == session2) {
                session2_results.push_back(view.payload.phase);
            }
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 5.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 2.0F;
    goto_cmd.yaw_rad = 0.2F;

    if (ground.command_publisher().publish_goto(
            peer1,
            session1,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 2),
            goto_cmd) != yunlink::ErrorCode::kOk) {
        std::cerr << "wrong-target publish failed unexpectedly\n";
        return 8;
    }
    if (!wait_until([&]() { return session1_results.size() == 1; })) {
        std::cerr << "wrong target did not produce failed command result\n";
        return 9;
    }
    if (session1_results.front() != yunlink::CommandPhase::kFailed) {
        std::cerr << "wrong target result was not failed\n";
        return 9;
    }

    if (ground.command_publisher().publish_goto(
            peer2,
            session2,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 2),
            goto_cmd) != yunlink::ErrorCode::kOk) {
        std::cerr << "correct target publish failed\n";
        return 10;
    }
    if (!wait_until([&]() { return session2_results.size() >= 4; })) {
        std::cerr << "correct target did not produce command results\n";
        return 11;
    }

    std::atomic<int> matching_states{0};
    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            if (message.envelope.source.agent_id == 2 &&
                message.payload.battery_percent == 66.0F) {
                ++matching_states;
            }
        });

    yunlink::VehicleCoreState state{};
    state.armed = true;
    state.nav_mode = 3;
    state.battery_percent = 66.0F;

    if (air2.publish_vehicle_core_state(
            air2_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 999),
            state,
            session2) != yunlink::ErrorCode::kOk) {
        std::cerr << "wrong-target state publish failed\n";
        return 12;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (matching_states.load() != 0) {
        std::cerr << "wrong-target state leaked to ground\n";
        return 13;
    }

    if (air2.publish_vehicle_core_state(
            air2_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 7),
            state,
            session2) != yunlink::ErrorCode::kOk) {
        std::cerr << "correct-target state publish failed\n";
        return 14;
    }
    if (!wait_until([&]() { return matching_states.load() == 1; })) {
        std::cerr << "correct-target state did not arrive\n";
        return 15;
    }

    ground.state_subscriber().unsubscribe(state_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air2.stop();
    air1.stop();
    return 0;
}
