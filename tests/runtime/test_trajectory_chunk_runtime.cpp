/**
 * @file tests/runtime/test_trajectory_chunk_runtime.cpp
 * @brief Trajectory chunk ordering, buffering and result aggregation tests.
 */

#include <chrono>
#include <cstdint>
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

yunlink::TrajectoryPoint point(float x, uint32_t dt_ms) {
    yunlink::TrajectoryPoint out{};
    out.x_m = x;
    out.y_m = x + 1.0F;
    out.z_m = x + 2.0F;
    out.dt_ms = dt_ms;
    return out;
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

uint64_t now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

const char* phase_name(yunlink::CommandPhase phase) {
    switch (phase) {
    case yunlink::CommandPhase::kReceived:
        return "Received";
    case yunlink::CommandPhase::kAccepted:
        return "Accepted";
    case yunlink::CommandPhase::kInProgress:
        return "InProgress";
    case yunlink::CommandPhase::kSucceeded:
        return "Succeeded";
    case yunlink::CommandPhase::kFailed:
        return "Failed";
    case yunlink::CommandPhase::kCancelled:
        return "Cancelled";
    case yunlink::CommandPhase::kExpired:
        return "Expired";
    }
    return "Unknown";
}

void dump_results_for_correlation(const std::vector<yunlink::CommandResultView>& results,
                                  uint64_t correlation_id) {
    bool found = false;
    for (const auto& result : results) {
        if (result.envelope.correlation_id != correlation_id) {
            continue;
        }
        found = true;
        std::cerr << "  result correlation_id=" << result.envelope.correlation_id
                  << " message_id=" << result.envelope.message_id
                  << " phase=" << phase_name(result.payload.phase)
                  << " code=" << result.payload.result_code << " detail=" << result.payload.detail
                  << "\n";
    }
    if (!found) {
        std::cerr << "  no command result observed for correlation_id=" << correlation_id << "\n";
    }
}

void dump_recent_results(const std::vector<yunlink::CommandResultView>& results, size_t limit = 8) {
    if (results.empty()) {
        std::cerr << "  recent results: <empty>\n";
        return;
    }

    const size_t start = results.size() > limit ? results.size() - limit : 0;
    std::cerr << "  recent results (" << (results.size() - start) << " of " << results.size()
              << "):\n";
    for (size_t i = start; i < results.size(); ++i) {
        const auto& result = results[i];
        std::cerr << "    [" << i << "] correlation_id=" << result.envelope.correlation_id
                  << " message_id=" << result.envelope.message_id
                  << " phase=" << phase_name(result.payload.phase)
                  << " code=" << result.payload.result_code << " detail=" << result.payload.detail
                  << "\n";
    }
}

void dump_authority_state(yunlink::Runtime& runtime, const yunlink::TargetSelector& target) {
    yunlink::AuthorityLease lease{};
    if (!runtime.current_authority_for_target(target, &lease)) {
        std::cerr << "  authority state: missing\n";
        return;
    }

    const uint64_t now_ms = now_millis();
    const uint64_t remaining_ms = lease.expires_at_ms > now_ms ? lease.expires_at_ms - now_ms : 0;
    std::cerr << "  authority state: present session_id=" << lease.session_id
              << " ttl_ms=" << lease.lease_ttl_ms << " expires_at_ms=" << lease.expires_at_ms
              << " remaining_ms=" << remaining_ms << "\n";
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12560;
    air_cfg.udp_target_port = 12560;
    air_cfg.tcp_listen_port = 12660;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 6;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "trajectory-runtime-secret";
    air_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;
    air_cfg.trajectory_chunk_timeout_ms = 1000;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12561;
    ground_cfg.udp_target_port = 12561;
    ground_cfg.tcp_listen_port = 12661;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 60;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "trajectory-runtime-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "planner");
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 6);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 5000) !=
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
    std::vector<yunlink::CommandResultView> results;
    int dispatch_count = 0;
    yunlink::TrajectoryChunkCommand assembled{};

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });

    const size_t cmd_token = air.command_subscriber().subscribe_trajectory_chunk(
        [&](const yunlink::InboundCommandView<yunlink::TrajectoryChunkCommand>& view) {
            {
                std::lock_guard<std::mutex> lock(mu);
                ++dispatch_count;
                assembled = view.payload;
            }

            yunlink::CommandResult result{};
            result.command_kind = yunlink::CommandKind::kTrajectoryChunk;
            result.phase = yunlink::CommandPhase::kSucceeded;
            result.progress_percent = 100;
            result.detail = "trajectory-executor-succeeded";
            (void)air.reply_command_result(view.inbound, result);
        });

    yunlink::TrajectoryChunkCommand missing_first{};
    missing_first.chunk_index = 1;
    missing_first.final_chunk = true;
    missing_first.points.push_back(point(90.0F, 10));

    yunlink::CommandHandle missing_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, missing_first, &missing_handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish missing-first trajectory failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     missing_handle.message_id,
                                     yunlink::CommandPhase::kFailed,
                                     "trajectory-missing-chunk");
        })) {
        std::cerr << "missing first chunk did not fail stably\n";
        return 6;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (dispatch_count != 0) {
            std::cerr << "out-of-order trajectory reached executor\n";
            return 7;
        }
    }

    yunlink::TrajectoryChunkCommand chunk0{};
    chunk0.chunk_index = 0;
    chunk0.final_chunk = false;
    chunk0.points.push_back(point(1.0F, 20));

    yunlink::CommandHandle chunk0_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, chunk0, &chunk0_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish first trajectory chunk failed\n";
        return 8;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     chunk0_handle.message_id,
                                     yunlink::CommandPhase::kAccepted,
                                     "trajectory-chunk-buffered");
        })) {
        std::cerr << "first trajectory chunk was not buffered and acked\n";
        return 9;
    }

    yunlink::CommandHandle duplicate_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, chunk0, &duplicate_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish duplicate trajectory chunk failed\n";
        return 10;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     duplicate_handle.message_id,
                                     yunlink::CommandPhase::kFailed,
                                     "trajectory-duplicate-chunk");
        })) {
        std::cerr << "duplicate trajectory chunk did not fail stably\n";
        return 11;
    }

    yunlink::TrajectoryChunkCommand final_chunk{};
    final_chunk.chunk_index = 1;
    final_chunk.final_chunk = true;
    final_chunk.points.push_back(point(2.0F, 30));

    yunlink::CommandHandle final_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, final_chunk, &final_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish final trajectory chunk failed\n";
        return 12;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return dispatch_count == 1 && has_result_detail(results,
                                                            final_handle.message_id,
                                                            yunlink::CommandPhase::kSucceeded,
                                                            "trajectory-executor-succeeded");
        })) {
        std::cerr << "completed trajectory was not dispatched once\n";
        return 13;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (assembled.chunk_index != 0 || !assembled.final_chunk || assembled.points.size() != 2 ||
            assembled.points[0].x_m != 1.0F || assembled.points[1].x_m != 2.0F) {
            std::cerr << "assembled trajectory payload mismatch\n";
            return 14;
        }
    }

    yunlink::TrajectoryChunkCommand timeout0{};
    timeout0.chunk_index = 0;
    timeout0.final_chunk = false;
    timeout0.points.push_back(point(5.0F, 20));
    yunlink::CommandHandle timeout0_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, timeout0, &timeout0_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish timeout first chunk failed\n";
        return 15;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     timeout0_handle.message_id,
                                     yunlink::CommandPhase::kAccepted,
                                     "trajectory-chunk-buffered");
        })) {
        std::cerr << "timeout first chunk was not buffered\n";
        return 16;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1200));

    yunlink::TrajectoryChunkCommand timeout1{};
    timeout1.chunk_index = 1;
    timeout1.final_chunk = true;
    timeout1.points.push_back(point(6.0F, 20));
    yunlink::CommandHandle timeout1_handle{};
    if (ground.command_publisher().publish_trajectory_chunk(
            peer_id, session_id, target, timeout1, &timeout1_handle) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish timeout final chunk failed\n";
        return 17;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_result_detail(results,
                                     timeout1_handle.message_id,
                                     yunlink::CommandPhase::kFailed,
                                     "trajectory-chunk-timeout");
        })) {
        std::cerr << "trajectory chunk timeout did not fail stably\n";
        {
            std::lock_guard<std::mutex> lock(mu);
            std::cerr << "debug timeout1 correlation_id=" << timeout1_handle.message_id << "\n";
            dump_results_for_correlation(results, timeout1_handle.message_id);
            dump_recent_results(results);
        }
        dump_authority_state(air, target);
        return 18;
    }

    air.command_subscriber().unsubscribe(cmd_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
