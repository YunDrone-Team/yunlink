/**
 * @file tests/runtime/test_command_result_from_status.cpp
 * @brief Runtime projection of terminal command execution status into CommandResult events.
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

struct ExpectedResult {
    yunlink::CommandKind kind = yunlink::CommandKind::kUnknown;
    uint64_t correlation_id = 0;
    yunlink::CommandPhase phase = yunlink::CommandPhase::kReceived;
};

bool has_result(const std::vector<yunlink::CommandResultView>& results,
                const ExpectedResult& expected) {
    for (const auto& result : results) {
        if (result.payload.command_kind == expected.kind &&
            result.envelope.correlation_id == expected.correlation_id &&
            result.payload.phase == expected.phase) {
            return true;
        }
    }
    return false;
}

size_t count_results(const std::vector<yunlink::CommandResultView>& results,
                     yunlink::CommandKind kind,
                     uint64_t correlation_id) {
    size_t count = 0;
    for (const auto& result : results) {
        if (result.payload.command_kind == kind &&
            result.envelope.correlation_id == correlation_id) {
            ++count;
        }
    }
    return count;
}

yunlink::CommandExecutionStatusSnapshot make_status(yunlink::CommandKind kind,
                                                    yunlink::CommandExecutionState state,
                                                    uint64_t command_correlation_id) {
    yunlink::CommandExecutionStatusSnapshot status{};
    status.agent_name = "uav-41";
    status.agent_id = 41;
    status.session_id = 91001;
    status.command_message_id = command_correlation_id + 1000;
    status.command_correlation_id = command_correlation_id;
    status.command_kind = kind;
    status.execution_state = static_cast<uint8_t>(state);
    status.progress_percent = 100;
    status.active = false;
    status.terminal = true;
    status.success = state == yunlink::CommandExecutionState::kSucceeded;
    status.detail = "status-derived-result";
    return status;
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12730;
    air_cfg.udp_target_port = 12730;
    air_cfg.tcp_listen_port = 12830;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 41;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "command-result-from-status-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12731;
    ground_cfg.udp_target_port = 12731;
    ground_cfg.tcp_listen_port = 12831;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 410;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "command-result-from-status-secret";

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

    const uint64_t session_id =
        ground.session_client().open_active_session(peer_id, "status-result-ground");
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 410);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); })) {
        std::cerr << "session setup failed\n";
        return 3;
    }

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session_id, &air_session) ||
        air_session.peer.id.empty()) {
        std::cerr << "air session peer missing\n";
        return 4;
    }

    std::mutex mu;
    std::vector<yunlink::CommandExecutionStatusSnapshot> statuses;
    std::vector<yunlink::CommandResultView> results;

    const size_t status_token = ground.state_subscriber().subscribe_command_execution_status(
        [&](const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message) {
            std::lock_guard<std::mutex> lock(mu);
            statuses.push_back(message.payload);
        });
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });

    auto publish = [&](const yunlink::CommandExecutionStatusSnapshot& status) {
        return air.publish_command_execution_status(
            air_session.peer.id, ground_target, status, status.session_id);
    };

    auto land =
        make_status(yunlink::CommandKind::kLand, yunlink::CommandExecutionState::kSucceeded, 70001);
    land.terminal = true;
    land.success = true;
    for (int i = 0; i < 100; ++i) {
        if (publish(land) != yunlink::ErrorCode::kOk) {
            std::cerr << "publish repeated land status failed\n";
            return 5;
        }
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return statuses.size() >= 100 &&
                   has_result(
                       results,
                       {yunlink::CommandKind::kLand, 70001, yunlink::CommandPhase::kSucceeded});
        })) {
        std::cerr << "land terminal status result not observed\n";
        return 6;
    }

    const std::vector<std::pair<yunlink::CommandExecutionStatusSnapshot, ExpectedResult>> cases = {
        {make_status(
             yunlink::CommandKind::kTakeoff, yunlink::CommandExecutionState::kSucceeded, 70002),
         {yunlink::CommandKind::kTakeoff, 70002, yunlink::CommandPhase::kSucceeded}},
        {make_status(yunlink::CommandKind::kReturn, yunlink::CommandExecutionState::kFailed, 70003),
         {yunlink::CommandKind::kReturn, 70003, yunlink::CommandPhase::kFailed}},
        {make_status(yunlink::CommandKind::kGoto, yunlink::CommandExecutionState::kTimeout, 70004),
         {yunlink::CommandKind::kGoto, 70004, yunlink::CommandPhase::kExpired}},
        {make_status(yunlink::CommandKind::kVelocitySetpoint,
                     yunlink::CommandExecutionState::kCancelled,
                     70005),
         {yunlink::CommandKind::kVelocitySetpoint, 70005, yunlink::CommandPhase::kCancelled}},
    };

    for (const auto& item : cases) {
        if (publish(item.first) != yunlink::ErrorCode::kOk) {
            std::cerr << "publish terminal status failed\n";
            return 7;
        }
    }

    auto running =
        make_status(yunlink::CommandKind::kGoto, yunlink::CommandExecutionState::kRunning, 70006);
    running.progress_percent = 50;
    running.active = true;
    running.terminal = false;
    running.success = false;
    if (publish(running) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish running status failed\n";
        return 8;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& item : cases) {
                if (!has_result(results, item.second)) {
                    return false;
                }
            }
            return true;
        })) {
        std::cerr << "terminal status result mapping not observed\n";
        return 9;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ground.state_subscriber().unsubscribe(status_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();

    std::lock_guard<std::mutex> lock(mu);
    if (count_results(results, yunlink::CommandKind::kLand, 70001) != 1) {
        std::cerr << "land repeated terminal status produced duplicate results\n";
        return 10;
    }
    if (count_results(results, yunlink::CommandKind::kTakeoff, 70002) != 1) {
        std::cerr << "takeoff terminal status result count mismatch\n";
        return 11;
    }
    if (count_results(results, yunlink::CommandKind::kGoto, 70006) != 0) {
        std::cerr << "running non-terminal status produced a result\n";
        return 12;
    }

    return 0;
}
