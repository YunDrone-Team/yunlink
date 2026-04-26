/**
 * @file tests/runtime/test_broadcast_policy.cpp
 * @brief Broadcast target message-family policy tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

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
    air_cfg.udp_bind_port = 12540;
    air_cfg.udp_target_port = 12540;
    air_cfg.tcp_listen_port = 12640;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "broadcast-policy-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12541;
    ground_cfg.udp_target_port = 12541;
    ground_cfg.tcp_listen_port = 12641;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 7;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "broadcast-policy-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ground");
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); })) {
        std::cerr << "session not active\n";
        return 3;
    }

    bool saw_broadcast_command_failure = false;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            if (view.payload.phase == yunlink::CommandPhase::kFailed &&
                view.payload.detail == "broadcast-command-disallowed") {
                saw_broadcast_command_failure = true;
            }
        });

    yunlink::GotoCommand goto_cmd{};
    if (ground.command_publisher().publish_goto(
            peer_id,
            session_id,
            yunlink::TargetSelector::broadcast(yunlink::AgentType::kUav),
            goto_cmd) != yunlink::ErrorCode::kOk) {
        std::cerr << "broadcast command publish failed before runtime policy\n";
        return 4;
    }
    if (!wait_until([&]() { return saw_broadcast_command_failure; })) {
        std::cerr << "broadcast command was not rejected by runtime policy\n";
        return 5;
    }

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session_id, &air_session)) {
        std::cerr << "air session missing\n";
        return 6;
    }

    bool saw_state = false;
    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            if (message.payload.battery_percent == 81.0F) {
                saw_state = true;
            }
        });

    yunlink::VehicleCoreState state{};
    state.battery_percent = 81.0F;
    if (air.publish_vehicle_core_state(
            air_session.peer.id,
            yunlink::TargetSelector::broadcast(yunlink::AgentType::kGroundStation),
            state,
            session_id) != yunlink::ErrorCode::kOk) {
        std::cerr << "broadcast state publish failed\n";
        return 7;
    }
    if (!wait_until([&]() { return saw_state; })) {
        std::cerr << "broadcast state did not arrive\n";
        return 8;
    }

    ground.state_subscriber().unsubscribe(state_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
