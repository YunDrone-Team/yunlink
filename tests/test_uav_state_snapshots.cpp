/**
 * @file tests/test_uav_state_snapshots.cpp
 * @brief UAV 专用上行快照语义回归测试。
 */

#include <atomic>
#include <chrono>
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
    yunlink::Runtime uav;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig uav_cfg;
    uav_cfg.udp_bind_port = 12340;
    uav_cfg.udp_target_port = 12340;
    uav_cfg.tcp_listen_port = 12440;
    uav_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    uav_cfg.self_identity.agent_id = 11;
    uav_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    uav_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12341;
    ground_cfg.udp_target_port = 12341;
    ground_cfg.tcp_listen_port = 12441;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 21;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "yunlink-secret";

    if (uav.start(uav_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", uav_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ground");
    if (session_id == 0 ||
        !wait_until([&]() { return uav.session_server().has_active_session(session_id); })) {
        std::cerr << "session not active\n";
        return 3;
    }

    yunlink::SessionDescriptor session{};
    if (!uav.session_server().describe_session(session_id, &session) || session.peer.id.empty()) {
        std::cerr << "session peer not resolved\n";
        return 4;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 21);

    std::atomic<int> received_count{0};
    yunlink::Px4StateSnapshot px4_seen{};
    yunlink::OdomStatusSnapshot odom_seen{};
    yunlink::UavControlFsmStateSnapshot fsm_seen{};
    yunlink::UavControllerStateSnapshot ctrl_seen{};
    yunlink::GimbalParamsSnapshot gimbal_seen{};
    yunlink::LocalOdomSnapshot local_odom_seen{};
    yunlink::UavControlStateSnapshot control_state_seen{};
    yunlink::OdomStateSnapshot odom_state_seen{};

    const size_t px4_token = ground.state_subscriber().subscribe_px4_state(
        [&](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& message) {
            px4_seen = message.payload;
            ++received_count;
        });
    const size_t odom_token = ground.state_subscriber().subscribe_odom_status(
        [&](const yunlink::TypedMessage<yunlink::OdomStatusSnapshot>& message) {
            odom_seen = message.payload;
            ++received_count;
        });
    const size_t fsm_token = ground.state_subscriber().subscribe_uav_control_fsm_state(
        [&](const yunlink::TypedMessage<yunlink::UavControlFsmStateSnapshot>& message) {
            fsm_seen = message.payload;
            ++received_count;
        });
    const size_t ctrl_token = ground.state_subscriber().subscribe_uav_controller_state(
        [&](const yunlink::TypedMessage<yunlink::UavControllerStateSnapshot>& message) {
            ctrl_seen = message.payload;
            ++received_count;
        });
    const size_t gimbal_token = ground.state_subscriber().subscribe_gimbal_params(
        [&](const yunlink::TypedMessage<yunlink::GimbalParamsSnapshot>& message) {
            gimbal_seen = message.payload;
            ++received_count;
        });
    const size_t local_odom_token = ground.state_subscriber().subscribe_local_odom(
        [&](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& message) {
            local_odom_seen = message.payload;
            ++received_count;
        });
    const size_t control_state_token = ground.state_subscriber().subscribe_uav_control_state(
        [&](const yunlink::TypedMessage<yunlink::UavControlStateSnapshot>& message) {
            control_state_seen = message.payload;
            ++received_count;
        });
    const size_t odom_state_token = ground.state_subscriber().subscribe_odom_state(
        [&](const yunlink::TypedMessage<yunlink::OdomStateSnapshot>& message) {
            odom_state_seen = message.payload;
            ++received_count;
        });

    yunlink::Px4StateSnapshot px4{};
    px4.connected = true;
    px4.armed = true;
    px4.flight_mode = 7;
    px4.landed_state = 2;
    px4.battery_voltage_v = 15.2F;
    px4.battery_current_a = 6.4F;
    px4.battery_percentage = 0.63F;
    px4.local_pose.position_m = {1.0F, 2.0F, 3.0F};
    px4.local_velocity.linear_mps = {0.1F, 0.2F, 0.3F};
    px4.yaw_setpoint_rad = 0.5F;
    px4.satellites = 14;
    px4.latitude_deg = 31.1234;

    yunlink::OdomStatusSnapshot odom{};
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

    yunlink::UavControlFsmStateSnapshot fsm{};
    fsm.takeoff_relative_height_m = 3.5;
    fsm.takeoff_max_velocity_mps = 1.2;
    fsm.land_type = 1;
    fsm.land_max_velocity_mps = 0.6;
    fsm.home_point_m = {8.0F, 9.0F, 10.0F};
    fsm.control_command = 6;
    fsm.yunlink_fsm_state = 6;

    yunlink::UavControllerStateSnapshot ctrl{};
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

    yunlink::GimbalParamsSnapshot gimbal{};
    gimbal.stream_type = 1;
    gimbal.encoding_type = 2;
    gimbal.resolution_width = 1920;
    gimbal.resolution_height = 1080;
    gimbal.bitrate_kbps = 4096;
    gimbal.frame_rate = 30.0F;

    yunlink::LocalOdomSnapshot local_odom{};
    local_odom.pose.position_m = {11.0F, 12.0F, 13.0F};
    local_odom.pose.orientation = {0.1F, 0.2F, 0.3F, 0.9F};
    local_odom.twist.linear_mps = {1.1F, 1.2F, 1.3F};

    yunlink::UavControlStateSnapshot control_state{};
    control_state.controller_types = 3;
    control_state.takeoff_relative_height_m = 2.8;
    control_state.takeoff_max_velocity_mps = 1.4;
    control_state.land_type = 1;
    control_state.land_max_velocity_mps = 0.7;
    control_state.home_point_m = {21.0F, 22.0F, 23.0F};
    control_state.control_state = 5;
    control_state.last_cmd.control_cmd = 8;
    control_state.last_cmd.cmd_source = 2;
    control_state.odometry_lost = false;
    control_state.odometry_valid = true;
    control_state.self_odom.pose.position_m.z = 2.2F;

    yunlink::OdomStateSnapshot odom_state{};
    odom_state.external_source = 2;
    odom_state.subtopic_name_external_odom = "/uav1/sunray/localization/external_odom";
    odom_state.odometry_valid = true;
    odom_state.odometry_update_hz = 48.0F;
    odom_state.subtopic_name_external_relocalization =
        "/uav1/sunray/localization/external_relocalization";
    odom_state.pubtopic_name_local_odom = "/uav1/sunray/localization/local_odom";
    odom_state.pubtopic_name_global_odom = "/uav1/sunray/localization/global_odom";
    odom_state.world_frame_name = "world";
    odom_state.global_frame_name = "map";
    odom_state.local_frame_name = "odom";
    odom_state.base_frame_name = "base_link";

    if (uav.publish_px4_state(session.peer.id, target, px4, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_odom_status(session.peer.id, target, odom, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_uav_control_fsm_state(session.peer.id, target, fsm, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_uav_controller_state(session.peer.id, target, ctrl, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_gimbal_params(session.peer.id, target, gimbal, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_local_odom(session.peer.id, target, local_odom, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_uav_control_state(session.peer.id, target, control_state, session_id) !=
            yunlink::ErrorCode::kOk ||
        uav.publish_odom_state(session.peer.id, target, odom_state, session_id) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "snapshot publish failed\n";
        return 5;
    }

    if (!wait_until([&]() { return received_count.load() == 8; })) {
        std::cerr << "not all snapshots received\n";
        return 6;
    }

    ground.state_subscriber().unsubscribe(px4_token);
    ground.state_subscriber().unsubscribe(odom_token);
    ground.state_subscriber().unsubscribe(fsm_token);
    ground.state_subscriber().unsubscribe(ctrl_token);
    ground.state_subscriber().unsubscribe(gimbal_token);
    ground.state_subscriber().unsubscribe(local_odom_token);
    ground.state_subscriber().unsubscribe(control_state_token);
    ground.state_subscriber().unsubscribe(odom_state_token);

    ground.stop();
    uav.stop();

    if (!px4_seen.connected || !px4_seen.armed || px4_seen.flight_mode != 7 ||
        px4_seen.local_pose.position_m.z != 3.0F || px4_seen.satellites != 14 ||
        px4_seen.battery_voltage_v != 15.2F || px4_seen.battery_current_a != 6.4F) {
        std::cerr << "px4 snapshot mismatch\n";
        return 7;
    }
    if (!odom_seen.has_odometry || odom_seen.local_frame_id != "odom" ||
        odom_seen.last_odometry_age_ms != 80) {
        std::cerr << "odom snapshot mismatch\n";
        return 8;
    }
    if (fsm_seen.control_command != 6 || fsm_seen.yunlink_fsm_state != 6 ||
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
    if (local_odom_seen.pose.position_m.x != 11.0F || local_odom_seen.pose.orientation.w != 0.9F ||
        local_odom_seen.twist.linear_mps.y != 1.2F) {
        std::cerr << "local odom snapshot mismatch\n";
        return 12;
    }
    if (control_state_seen.controller_types != 3 || !control_state_seen.odometry_valid ||
        control_state_seen.home_point_m.z != 23.0F ||
        control_state_seen.self_odom.pose.position_m.z != 2.2F) {
        std::cerr << "uav control state snapshot mismatch\n";
        return 13;
    }
    if (odom_state_seen.external_source != 2 || !odom_state_seen.odometry_valid ||
        odom_state_seen.pubtopic_name_local_odom != "/uav1/sunray/localization/local_odom" ||
        odom_state_seen.base_frame_name != "base_link") {
        std::cerr << "odom state snapshot mismatch\n";
        return 14;
    }

    return 0;
}
