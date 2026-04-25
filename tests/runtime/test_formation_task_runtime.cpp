/**
 * @file tests/runtime/test_formation_task_runtime.cpp
 * @brief Formation task group contract and member-level result policy tests.
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

bool has_result_detail(const std::vector<yunlink::CommandResultView>& results,
                       uint64_t correlation_id,
                       yunlink::CommandPhase phase,
                       const std::string& detail) {
    for (const auto& result : results) {
        if (result.envelope.correlation_id == correlation_id && result.payload.phase == phase &&
            result.payload.detail == detail) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    yunlink::Runtime swarm;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig swarm_cfg;
    swarm_cfg.udp_bind_port = 12570;
    swarm_cfg.udp_target_port = 12570;
    swarm_cfg.tcp_listen_port = 12670;
    swarm_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    swarm_cfg.self_identity.agent_id = 101;
    swarm_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    swarm_cfg.self_identity.group_ids = {77};
    swarm_cfg.shared_secret = "formation-runtime-secret";
    swarm_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12571;
    ground_cfg.udp_target_port = 12571;
    ground_cfg.tcp_listen_port = 12671;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 70;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "formation-runtime-secret";

    if (swarm.start(swarm_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", swarm_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_id =
        ground.session_client().open_active_session(peer_id, "formation-ground");
    const auto group_target = yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 77);
    if (session_id == 0 ||
        !wait_until([&]() { return swarm.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, group_target, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 3;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return swarm.current_authority_for_target(group_target, &lease); })) {
        std::cerr << "group authority not granted\n";
        return 4;
    }

    std::mutex mu;
    std::vector<yunlink::CommandResultView> results;
    int dispatch_count = 0;
    yunlink::FormationTaskCommand observed{};

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });

    const size_t task_token = swarm.command_subscriber().subscribe_formation_task(
        [&](const yunlink::InboundCommandView<yunlink::FormationTaskCommand>& view) {
            {
                std::lock_guard<std::mutex> lock(mu);
                ++dispatch_count;
                observed = view.payload;
            }

            yunlink::CommandResult received{};
            received.command_kind = yunlink::CommandKind::kFormationTask;
            received.phase = yunlink::CommandPhase::kReceived;
            received.detail = "formation-group-received";
            (void)swarm.reply_command_result(view.inbound, received);

            yunlink::CommandResult member_ok{};
            member_ok.command_kind = yunlink::CommandKind::kFormationTask;
            member_ok.phase = yunlink::CommandPhase::kInProgress;
            member_ok.progress_percent = 50;
            member_ok.detail = "member:101:in-progress";
            (void)swarm.reply_command_result(view.inbound, member_ok);

            yunlink::CommandResult aggregate{};
            aggregate.command_kind = yunlink::CommandKind::kFormationTask;
            aggregate.phase = yunlink::CommandPhase::kSucceeded;
            aggregate.progress_percent = 100;
            aggregate.detail = "formation-all-members-succeeded";
            (void)swarm.reply_command_result(view.inbound, aggregate);
        });

    yunlink::FormationTaskCommand wrong_scope{};
    wrong_scope.group_id = 77;
    wrong_scope.formation_shape = 1;
    wrong_scope.spacing_m = 3.5F;
    wrong_scope.label = "wrong-scope";

    yunlink::CommandHandle wrong_scope_handle{};
    if (ground.command_publisher().publish_formation_task(
            peer_id,
            session_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 101),
            wrong_scope,
            &wrong_scope_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish wrong-scope formation task failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     wrong_scope_handle.message_id,
                                     yunlink::CommandPhase::kFailed,
                                     "formation-target-not-group");
        })) {
        std::cerr << "formation task with entity target was not rejected\n";
        return 6;
    }

    yunlink::FormationTaskCommand mismatch{};
    mismatch.group_id = 99;
    mismatch.formation_shape = 1;
    mismatch.spacing_m = 3.5F;
    mismatch.label = "mismatch";
    yunlink::CommandHandle mismatch_handle{};
    if (ground.command_publisher().publish_formation_task(
            peer_id, session_id, group_target, mismatch, &mismatch_handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish mismatched formation task failed\n";
        return 7;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     mismatch_handle.message_id,
                                     yunlink::CommandPhase::kFailed,
                                     "formation-group-mismatch");
        })) {
        std::cerr << "formation group mismatch was not rejected\n";
        return 8;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (dispatch_count != 0) {
            std::cerr << "invalid formation task reached executor\n";
            return 9;
        }
    }

    yunlink::FormationTaskCommand valid{};
    valid.group_id = 77;
    valid.formation_shape = 2;
    valid.spacing_m = 4.0F;
    valid.label = "diamond";

    yunlink::CommandHandle valid_handle{};
    if (ground.command_publisher().publish_formation_task(
            peer_id, session_id, group_target, valid, &valid_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish valid formation task failed\n";
        return 10;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return dispatch_count == 1 &&
                   has_result_detail(results,
                                     valid_handle.message_id,
                                     yunlink::CommandPhase::kReceived,
                                     "formation-group-received") &&
                   has_result_detail(results,
                                     valid_handle.message_id,
                                     yunlink::CommandPhase::kInProgress,
                                     "member:101:in-progress") &&
                   has_result_detail(results,
                                     valid_handle.message_id,
                                     yunlink::CommandPhase::kSucceeded,
                                     "formation-all-members-succeeded");
        })) {
        std::cerr << "valid formation task result stream mismatch\n";
        return 11;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (observed.group_id != valid.group_id || observed.formation_shape != valid.formation_shape ||
            observed.spacing_m != valid.spacing_m || observed.label != valid.label) {
            std::cerr << "formation task payload mismatch\n";
            return 12;
        }
    }

    swarm.command_subscriber().unsubscribe(task_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    swarm.stop();
    return 0;
}
