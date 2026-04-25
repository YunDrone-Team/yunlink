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
    air_cfg.udp_bind_port = 12392;
    air_cfg.udp_target_port = 12392;
    air_cfg.tcp_listen_port = 12492;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12393;
    ground_cfg.udp_target_port = 12393;
    ground_cfg.tcp_listen_port = 12493;
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
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "stopped-ground");
    if (session_id == 0 || !wait_until([&]() { return air.session_server().has_active_session(session_id); })) {
        std::cerr << "session setup failed\n";
        return 3;
    }

    ground.stop();

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 3.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 2.0F;
    goto_cmd.yaw_rad = 0.1F;

    if (ground.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd) !=
        yunlink::ErrorCode::kRuntimeStopped) {
        std::cerr << "stopped command publish mismatch\n";
        return 4;
    }
    if (ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 1000) !=
        yunlink::ErrorCode::kRuntimeStopped) {
        std::cerr << "stopped authority request mismatch\n";
        return 5;
    }

    yunlink::VehicleCoreState state{};
    state.armed = true;
    state.battery_percent = 50.0F;
    if (ground.publish_vehicle_core_state(
            peer_id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 7),
            state,
            session_id) != yunlink::ErrorCode::kRuntimeStopped) {
        std::cerr << "stopped state publish mismatch\n";
        return 6;
    }

    air.stop();
    return 0;
}
