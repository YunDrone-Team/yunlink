/**
 * @file tests/runtime/test_authority_edges.cpp
 * @brief authority expiry / no-authority command / explicit recovery edge tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include "yunlink/core/semantic_messages.hpp"
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
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12350;
    air_cfg.udp_target_port = 12350;
    air_cfg.tcp_listen_port = 12450;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12351;
    ground_cfg.udp_target_port = 12351;
    ground_cfg.tcp_listen_port = 12451;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 7;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "yunlink-secret";

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ground");
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); })) {
        std::cerr << "session not active\n";
        return 3;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    std::vector<yunlink::CommandPhase> result_phases;
    std::vector<yunlink::CommandResultView> result_views;
    bool saw_no_active_session = false;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            if (view.envelope.session_id == 9999 &&
                view.payload.phase == yunlink::CommandPhase::kFailed &&
                view.payload.detail == "no-active-session") {
                saw_no_active_session = true;
            }
            if (view.envelope.session_id == session_id) {
                result_phases.push_back(view.payload.phase);
                result_views.push_back(view);
            }
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 3.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 2.0F;
    goto_cmd.yaw_rad = 0.1F;

    yunlink::CommandHandle handle{};
    if (ground.command_publisher().publish_goto(peer_id, 9999, target, goto_cmd, &handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish with missing session failed unexpectedly\n";
        return 4;
    }
    if (!wait_until([&]() { return saw_no_active_session; })) {
        std::cerr << "missing-session command failure was not emitted\n";
        return 5;
    }

    if (ground.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
            yunlink::ErrorCode::kOk ||
        handle.message_id == 0) {
        std::cerr << "publish without authority failed unexpectedly\n";
        return 6;
    }

    if (!wait_until([&]() { return !result_views.empty(); })) {
        std::cerr << "no-authority command failure was not emitted\n";
        return 7;
    }
    if (result_views.front().payload.phase != yunlink::CommandPhase::kFailed ||
        result_views.front().payload.detail != "no-authority") {
        std::cerr << "no-authority command failure mismatch\n";
        return 8;
    }
    result_phases.clear();
    result_views.clear();

    if (ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 120) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 9;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() {
            return air.current_authority(&lease) && lease.session_id == session_id &&
                   lease.lease_ttl_ms == 120;
        })) {
        std::cerr << "authority was not granted\n";
        return 10;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(220));
    if (air.current_authority(&lease)) {
        std::cerr << "authority lease did not expire\n";
        return 11;
    }

    if (ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 800) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority reacquire failed\n";
        return 12;
    }
    if (!wait_until([&]() { return air.current_authority(&lease) && lease.session_id == session_id; })) {
        std::cerr << "authority reacquire not observed\n";
        return 13;
    }

    if (ground.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish with authority failed\n";
        return 14;
    }
    if (!wait_until([&]() { return result_phases.size() >= 4; })) {
        std::cerr << "command result flow not observed after reacquire\n";
        return 15;
    }

    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();

    const std::vector<yunlink::CommandPhase> expected = {
        yunlink::CommandPhase::kReceived,
        yunlink::CommandPhase::kAccepted,
        yunlink::CommandPhase::kInProgress,
        yunlink::CommandPhase::kSucceeded,
    };
    if (result_phases.size() < expected.size() ||
        !std::equal(expected.begin(), expected.end(), result_phases.begin())) {
        std::cerr << "command phase flow mismatch\n";
        return 16;
    }

    return 0;
}
