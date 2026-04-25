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
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12390;
    air_cfg.udp_target_port = 12390;
    air_cfg.tcp_listen_port = 12490;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12391;
    ground_cfg.udp_target_port = 12391;
    ground_cfg.tcp_listen_port = 12491;
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
        std::cerr << "initial connect failed\n";
        return 2;
    }

    const uint64_t session1 = ground.session_client().open_active_session(peer_id, "restart-ground");
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (session1 == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session1); }) ||
        ground.request_authority(
            peer_id, session1, target, yunlink::ControlSource::kGroundStation, 1000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "initial session setup failed\n";
        return 3;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air.current_authority(&lease) && lease.session_id == session1; })) {
        std::cerr << "initial authority missing\n";
        return 4;
    }

    ground.stop();
    air.stop();

    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, nullptr) !=
        yunlink::ErrorCode::kRuntimeStopped) {
        std::cerr << "stopped runtime did not reject connect\n";
        return 5;
    }

    if (air.current_authority(&lease)) {
        std::cerr << "authority survived stop\n";
        return 6;
    }
    if (air.session_server().has_active_session(session1)) {
        std::cerr << "session survived stop\n";
        return 7;
    }

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime restart failed\n";
        return 8;
    }
    if (air.current_authority(&lease)) {
        std::cerr << "authority appeared before reacquire\n";
        return 9;
    }

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 8.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 2.0F;
    goto_cmd.yaw_rad = 0.25F;
    if (ground.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kConnectError) {
        std::cerr << "old peer/session unexpectedly survived restart\n";
        return 10;
    }

    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "reconnect failed\n";
        return 11;
    }

    const uint64_t session2 = ground.session_client().open_active_session(peer_id, "restart-ground");
    if (session2 == 0 || session2 == session1 ||
        !wait_until([&]() { return air.session_server().has_active_session(session2); }) ||
        air.session_server().has_active_session(session1)) {
        std::cerr << "session reopen did not require explicit recovery\n";
        return 12;
    }

    if (ground.request_authority(
            peer_id, session2, target, yunlink::ControlSource::kGroundStation, 1000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority reacquire failed\n";
        return 13;
    }
    if (!wait_until([&]() { return air.current_authority(&lease) && lease.session_id == session2; })) {
        std::cerr << "authority reacquire missing\n";
        return 14;
    }

    std::vector<yunlink::CommandPhase> phases;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            if (view.envelope.session_id == session2) {
                phases.push_back(view.payload.phase);
            }
        });

    if (ground.command_publisher().publish_goto(peer_id, session2, target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "post-restart command publish failed\n";
        return 15;
    }
    if (!wait_until([&]() { return phases.size() >= 4; })) {
        std::cerr << "post-restart command flow missing\n";
        return 16;
    }

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session2, &air_session) ||
        air_session.peer.id.empty()) {
        std::cerr << "restarted session peer missing\n";
        return 17;
    }

    bool saw_state = false;
    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            if (message.envelope.session_id == session2 &&
                message.payload.battery_percent == 72.0F) {
                saw_state = true;
            }
        });

    yunlink::VehicleCoreState state{};
    state.armed = true;
    state.nav_mode = 2;
    state.battery_percent = 72.0F;
    if (air.publish_vehicle_core_state(
            air_session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 7),
            state,
            session2) != yunlink::ErrorCode::kOk) {
        std::cerr << "post-restart state publish failed\n";
        return 18;
    }
    if (!wait_until([&]() { return saw_state; })) {
        std::cerr << "post-restart state flow missing\n";
        return 19;
    }

    ground.state_subscriber().unsubscribe(state_token);
    ground.event_subscriber().unsubscribe(result_token);

    ground.stop();
    air.stop();
    return 0;
}
