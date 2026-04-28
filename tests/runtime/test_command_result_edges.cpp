/**
 * @file tests/runtime/test_command_result_edges.cpp
 * @brief Explicit command result phase and recovery-edge regression tests.
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

bool phases_match_prefix(const std::vector<yunlink::CommandPhase>& actual,
                         const std::vector<yunlink::CommandPhase>& expected) {
    return actual.size() >= expected.size() &&
           std::equal(expected.begin(), expected.end(), actual.begin());
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12620;
    air_cfg.udp_target_port = 12620;
    air_cfg.tcp_listen_port = 12720;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 31;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "command-result-edges-secret";
    air_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12621;
    ground_cfg.udp_target_port = 12621;
    ground_cfg.tcp_listen_port = 12721;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 310;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "command-result-edges-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "phase-ground");
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 31);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 3000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 3;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air.current_authority_for_target(target, &lease); })) {
        std::cerr << "authority not granted\n";
        return 4;
    }

    std::mutex mu;
    std::vector<yunlink::CommandResultView> views;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            if (view.envelope.session_id == session_id) {
                views.push_back(view);
            }
        });

    const size_t handler_token = air.command_subscriber().subscribe_goto(
        [&](const yunlink::InboundCommandView<yunlink::GotoCommand>& view) {
            auto reply_phase = [&](yunlink::CommandPhase phase,
                                   uint8_t progress,
                                   const std::string& detail) {
                yunlink::CommandResult result{};
                result.command_kind = yunlink::CommandKind::kGoto;
                result.phase = phase;
                result.progress_percent = progress;
                result.detail = detail;
                if (air.reply_command_result(view.inbound, result) != yunlink::ErrorCode::kOk) {
                    std::cerr << "reply_command_result failed\n";
                }
            };

            reply_phase(yunlink::CommandPhase::kReceived, 0, "manual-received");
            reply_phase(yunlink::CommandPhase::kAccepted, 10, "manual-accepted");
            reply_phase(yunlink::CommandPhase::kInProgress, 80, "manual-progress");
            reply_phase(yunlink::CommandPhase::kCancelled, 100, "manual-cancelled");
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 8.0F;
    goto_cmd.y_m = 1.5F;
    goto_cmd.z_m = 3.5F;
    goto_cmd.yaw_rad = 0.6F;

    yunlink::CommandHandle handle{};
    if (ground.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
            yunlink::ErrorCode::kOk ||
        handle.message_id == 0) {
        std::cerr << "publish goto failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return views.size() >= 4;
        })) {
        std::cerr << "manual command phase sequence not observed\n";
        return 6;
    }

    ground.event_subscriber().unsubscribe(result_token);
    air.command_subscriber().unsubscribe(handler_token);
    ground.stop();
    air.stop();

    std::lock_guard<std::mutex> lock(mu);
    const std::vector<yunlink::CommandPhase> expected = {
        yunlink::CommandPhase::kReceived,
        yunlink::CommandPhase::kAccepted,
        yunlink::CommandPhase::kInProgress,
        yunlink::CommandPhase::kCancelled,
    };
    std::vector<yunlink::CommandPhase> actual;
    for (const auto& view : views) {
        actual.push_back(view.payload.phase);
        if (view.envelope.correlation_id != handle.message_id) {
            std::cerr << "command result correlation mismatch\n";
            return 7;
        }
    }
    if (!phases_match_prefix(actual, expected)) {
        std::cerr << "manual command phase prefix mismatch\n";
        return 8;
    }
    if (views[0].payload.detail != "manual-received" ||
        views[1].payload.detail != "manual-accepted" ||
        views[2].payload.detail != "manual-progress" ||
        views[3].payload.detail != "manual-cancelled") {
        std::cerr << "manual command phase detail mismatch\n";
        return 9;
    }

    return 0;
}
