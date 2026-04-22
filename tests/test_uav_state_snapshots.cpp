/**
 * @file tests/test_uav_state_snapshots.cpp
 * @brief UAV 专用上行快照语义回归测试。
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "sunraycom/runtime/runtime.hpp"

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
    sunraycom::Runtime uav;
    sunraycom::Runtime ground;

    sunraycom::RuntimeConfig uav_cfg;
    uav_cfg.udp_bind_port = 12340;
    uav_cfg.udp_target_port = 12340;
    uav_cfg.tcp_listen_port = 12440;
    uav_cfg.self_identity.agent_type = sunraycom::AgentType::kUav;
    uav_cfg.self_identity.agent_id = 11;
    uav_cfg.self_identity.role = sunraycom::EndpointRole::kVehicle;
    uav_cfg.shared_secret = "sunray-secret";

    sunraycom::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12341;
    ground_cfg.udp_target_port = 12341;
    ground_cfg.tcp_listen_port = 12441;
    ground_cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 21;
    ground_cfg.self_identity.role = sunraycom::EndpointRole::kController;
    ground_cfg.shared_secret = "sunray-secret";

    if (uav.start(uav_cfg) != sunraycom::ErrorCode::kOk ||
        ground.start(ground_cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", uav_cfg.tcp_listen_port, &peer_id) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ground");
    if (session_id == 0 ||
        !wait_until([&]() { return uav.session_server().has_active_session(session_id); })) {
        std::cerr << "session not active\n";
        return 3;
    }

    sunraycom::SessionDescriptor session{};
    if (!uav.session_server().describe_session(session_id, &session) || session.peer.id.empty()) {
        std::cerr << "session peer not resolved\n";
        return 4;
    }

    const auto target =
        sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kGroundStation, 21);

    std::atomic<int> received_count{0};
    sunraycom::Px4StateSnapshot px4_seen{};
    sunraycom::OdomStatusSnapshot odom_seen{};
    sunraycom::UavControlFsmStateSnapshot fsm_seen{};
    sunraycom::UavControllerStateSnapshot ctrl_seen{};
    sunraycom::GimbalParamsSnapshot gimbal_seen{};

    const size_t px4_token = ground.state_subscriber().subscribe_px4_state(
        [&](const sunraycom::TypedMessage<sunraycom::Px4StateSnapshot>& message) {
            px4_seen = message.payload;
            ++received_count;
        });
    const size_t odom_token = ground.state_subscriber().subscribe_odom_status(
        [&](const sunraycom::TypedMessage<sunraycom::OdomStatusSnapshot>& message) {
            odom_seen = message.payload;
            ++received_count;
        });
    const size_t fsm_token = ground.state_subscriber().subscribe_uav_control_fsm_state(
        [&](const sunraycom::TypedMessage<sunraycom::UavControlFsmStateSnapshot>& message) {
            fsm_seen = message.payload;
            ++received_count;
        });
    const size_t ctrl_token = ground.state_subscriber().subscribe_uav_controller_state(
        [&](const sunraycom::TypedMessage<sunraycom::UavControllerStateSnapshot>& message) {
            ctrl_seen = message.payload;
            ++received_count;
        });
    const size_t gimbal_token = ground.state_subscriber().subscribe_gimbal_params(
        [&](const sunraycom::TypedMessage<sunraycom::GimbalParamsSnapshot>& message) {
            gimbal_seen = message.payload;
            ++received_count;
        });

    sunraycom::Px4StateSnapshot px4{};
    px4.connected = true;
    px4.armed = true;
    px4.flight_mode = 7;
    px4.flight_mode_name = "OFFBOARD";
    px4.landed_state = 2;
    px4.battery_percentage = 0.63F;
    px4.local_position_m = {1.0F, 2.0F, 3.0F};
    px4.local_velocity_mps = {0.1F, 0.2F, 0.3F};
    px4.yaw_setpoint_rad = 0.5F;
    px4.satellites = 14;
    px4.latitude_deg = 31.1234;

    sunraycom::OdomStatusSnapshot odom{};
    odom.external_source_name = "VIOBOT2";
    odom.external_source_id = 0;
    odom.localization_mode_name = "LOCAL_AND_GLOBAL";
    odom.localization_mode = 2;
    odom.has_odometry = true;
    odom.has_relocalization = true;
    odom.odom_timeout = false;
    odom.relocalization_data_valid = true;
    odom.last_odometry_age_ms = 80;
    odom.global_frame_id = "map";
    odom.local_frame_id = "odom";
    odom.base_frame_id = "base_link";

    sunraycom::UavControlFsmStateSnapshot fsm{};
    fsm.takeoff_relative_height_m = 3.5;
    fsm.takeoff_max_velocity_mps = 1.2;
    fsm.land_type = 1;
    fsm.land_max_velocity_mps = 0.6;
    fsm.home_point_m = {8.0F, 9.0F, 10.0F};
    fsm.control_command = 6;
    fsm.sunray_fsm_state = 6;

    sunraycom::UavControllerStateSnapshot ctrl{};
    ctrl.reference_frame = 0;
    ctrl.controller_type = 1;
    ctrl.desired_position_m = {5.0F, 6.0F, 7.0F};
    ctrl.current_position_m = {4.5F, 5.5F, 6.5F};
    ctrl.desired_velocity_mps = {0.4F, 0.5F, 0.6F};
    ctrl.current_velocity_mps = {0.1F, 0.2F, 0.3F};
    ctrl.position_error_m = {0.5F, 0.5F, 0.5F};
    ctrl.velocity_error_mps = {0.3F, 0.3F, 0.3F};
    ctrl.desired_yaw_rad = 1.1;
    ctrl.current_yaw_rad = 1.0;
    ctrl.yaw_error_rad = 0.1;
    ctrl.thrust_from_px4 = 0.55;
    ctrl.thrust_from_controller = 0.58;

    sunraycom::GimbalParamsSnapshot gimbal{};
    gimbal.stream_type = 1;
    gimbal.encoding_type = 2;
    gimbal.resolution_width = 1920;
    gimbal.resolution_height = 1080;
    gimbal.bitrate_kbps = 4096;
    gimbal.frame_rate = 30.0F;

    if (uav.publish_px4_state(session.peer.id, target, px4, session_id) !=
            sunraycom::ErrorCode::kOk ||
        uav.publish_odom_status(session.peer.id, target, odom, session_id) !=
            sunraycom::ErrorCode::kOk ||
        uav.publish_uav_control_fsm_state(session.peer.id, target, fsm, session_id) !=
            sunraycom::ErrorCode::kOk ||
        uav.publish_uav_controller_state(session.peer.id, target, ctrl, session_id) !=
            sunraycom::ErrorCode::kOk ||
        uav.publish_gimbal_params(session.peer.id, target, gimbal, session_id) !=
            sunraycom::ErrorCode::kOk) {
        std::cerr << "snapshot publish failed\n";
        return 5;
    }

    if (!wait_until([&]() { return received_count.load() == 5; })) {
        std::cerr << "not all snapshots received\n";
        return 6;
    }

    ground.state_subscriber().unsubscribe(px4_token);
    ground.state_subscriber().unsubscribe(odom_token);
    ground.state_subscriber().unsubscribe(fsm_token);
    ground.state_subscriber().unsubscribe(ctrl_token);
    ground.state_subscriber().unsubscribe(gimbal_token);

    ground.stop();
    uav.stop();

    if (!px4_seen.connected || !px4_seen.armed || px4_seen.flight_mode_name != "OFFBOARD" ||
        px4_seen.local_position_m.z != 3.0F || px4_seen.satellites != 14) {
        std::cerr << "px4 snapshot mismatch\n";
        return 7;
    }
    if (!odom_seen.has_odometry || odom_seen.local_frame_id != "odom" ||
        odom_seen.last_odometry_age_ms != 80) {
        std::cerr << "odom snapshot mismatch\n";
        return 8;
    }
    if (fsm_seen.control_command != 6 || fsm_seen.sunray_fsm_state != 6 ||
        fsm_seen.home_point_m.x != 8.0F) {
        std::cerr << "fsm snapshot mismatch\n";
        return 9;
    }
    if (ctrl_seen.controller_type != 1 || ctrl_seen.position_error_m.x != 0.5F ||
        ctrl_seen.thrust_from_controller != 0.58) {
        std::cerr << "controller snapshot mismatch\n";
        return 10;
    }
    if (gimbal_seen.encoding_type != 2 || gimbal_seen.resolution_width != 1920 ||
        gimbal_seen.frame_rate != 30.0F) {
        std::cerr << "gimbal snapshot mismatch\n";
        return 11;
    }

    return 0;
}
