/**
 * @file tests/runtime/test_bulk_channel_runtime.cpp
 * @brief Bulk descriptor delivery and lifecycle isolation tests.
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

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12580;
    air_cfg.udp_target_port = 12580;
    air_cfg.tcp_listen_port = 12680;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 8;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "bulk-runtime-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12581;
    ground_cfg.udp_target_port = 12581;
    ground_cfg.tcp_listen_port = 12681;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 80;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "bulk-runtime-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "bulk-ground");
    const auto air_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 8);
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 80);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, air_target, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 3;
    }

    std::mutex mu;
    std::vector<yunlink::BulkChannelDescriptor> descriptors;
    std::vector<yunlink::CommandPhase> command_phases;

    const size_t bulk_token = ground.event_subscriber().subscribe_bulk_channel_descriptors(
        [&](const yunlink::TypedMessage<yunlink::BulkChannelDescriptor>& message) {
            std::lock_guard<std::mutex> lock(mu);
            descriptors.push_back(message.payload);
        });

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            command_phases.push_back(view.payload.phase);
        });

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session_id, &air_session) ||
        air_session.peer.id.empty()) {
        std::cerr << "air session peer missing\n";
        return 4;
    }

    yunlink::BulkChannelDescriptor ready{};
    ready.channel_id = 42;
    ready.stream_type = yunlink::BulkStreamType::kVideo;
    ready.state = yunlink::BulkChannelState::kReady;
    ready.uri = "udp://239.1.1.42:5600";
    ready.mtu_bytes = 1200;
    ready.reliable = false;
    ready.detail = "video-ready";

    if (air.publish_bulk_channel_descriptor(
            air_session.peer.id, ground_target, ready, session_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish ready bulk descriptor failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            yunlink::BulkChannelDescriptor active{};
            return descriptors.size() == 1 && ground.current_bulk_channel(42, &active) &&
                   active.uri == ready.uri && active.state == yunlink::BulkChannelState::kReady;
        })) {
        std::cerr << "ready bulk descriptor was not delivered/registered\n";
        return 6;
    }

    yunlink::BulkChannelDescriptor failed = ready;
    failed.state = yunlink::BulkChannelState::kFailed;
    failed.uri.clear();
    failed.detail = "bulk-sidecar-failed";
    if (air.publish_bulk_channel_descriptor(
            air_session.peer.id, ground_target, failed, session_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "publish failed bulk descriptor failed\n";
        return 7;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            yunlink::BulkChannelDescriptor active{};
            return descriptors.size() == 2 && !ground.current_bulk_channel(42, &active) &&
                   descriptors.back().state == yunlink::BulkChannelState::kFailed &&
                   descriptors.back().detail == "bulk-sidecar-failed";
        })) {
        std::cerr << "failed bulk descriptor was not delivered/unregistered\n";
        return 8;
    }

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 1.0F;
    goto_cmd.y_m = 2.0F;
    goto_cmd.z_m = 3.0F;
    if (ground.command_publisher().publish_goto(peer_id, session_id, air_target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "command publish failed after bulk failure\n";
        return 9;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return command_phases.size() >= 4 &&
                   command_phases.back() == yunlink::CommandPhase::kSucceeded;
        })) {
        std::cerr << "bulk failure blocked main command plane\n";
        return 10;
    }

    ground.event_subscriber().unsubscribe(result_token);
    ground.event_subscriber().unsubscribe(bulk_token);
    ground.stop();
    air.stop();
    return 0;
}
