#include "mapping/value_map.hpp"

#include "common/sunray_status_format.hpp"
#include "mapping/value_setters.hpp"

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_odometry(values, "", msg);
}

void fill_odom_state_from_yunlink(const yunlink::OdomStateSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_numeric(values, "external_source", msg.external_source);
    set_value(values, "external_source_name", localization_source_name(msg.external_source));
    set_value(values, "subtopic_name_external_odom", msg.subtopic_name_external_odom);
    set_value(values, "odometry_valid", monitor_fmt_bool(msg.odometry_valid));
    set_float(values, "odometry_update_hz", msg.odometry_update_hz);
    set_value(values,
              "subtopic_name_external_relocalization",
              msg.subtopic_name_external_relocalization);
    set_value(values, "pubtopic_name_local_odom", msg.pubtopic_name_local_odom);
    set_value(values, "pubtopic_name_global_odom", msg.pubtopic_name_global_odom);
    set_odometry(values, "local_odom.", msg.local_odom);
    set_odometry(values, "global_odom.", msg.global_odom);
    set_value(values, "world_frame_name", msg.world_frame_name);
    set_value(values, "global_frame_name", msg.global_frame_name);
    set_value(values, "local_frame_name", msg.local_frame_name);
    set_value(values, "base_frame_name", msg.base_frame_name);
    set_transform(values, "world_to_global_tf.", msg.world_to_global_tf);
    set_transform(values, "global_to_local_tf.", msg.global_to_local_tf);
    set_transform(values, "local_to_base_tf.", msg.local_to_base_tf);
}

void fill_control_cmd_from_yunlink(const yunlink::UavControlCmdSnapshot& msg,
                                   std::unordered_map<std::string, std::string>& values) {
    set_control_cmd(values, "", msg);
    set_value(values, "cmd_source_name", uav_control_cmd_source_name(msg.cmd_source));
    set_value(values, "control_cmd_name", uav_control_cmd_name(msg.control_cmd));
    set_value(values, "yaw_mode_name", uav_yaw_mode_name(msg.yaw_mode));
}

void fill_control_state_from_yunlink(const yunlink::UavControlStateSnapshot& msg,
                                     std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "agent_name", msg.agent_name);
    set_numeric(values, "agent_id", msg.agent_id);
    set_numeric(values, "controller_types", msg.controller_types);
    set_value(values, "controller_types_name", uav_controller_type_name(msg.controller_types));
    set_float(values, "takeoff_relative_height_m", msg.takeoff_relative_height_m);
    set_float(values, "takeoff_max_velocity_mps", msg.takeoff_max_velocity_mps);
    set_numeric(values, "land_type", msg.land_type);
    set_value(values, "land_type_name", land_type_name(msg.land_type));
    set_float(values, "land_max_velocity_mps", msg.land_max_velocity_mps);
    set_float(values, "home_point_m.x", msg.home_point_m.x);
    set_float(values, "home_point_m.y", msg.home_point_m.y);
    set_float(values, "home_point_m.z", msg.home_point_m.z);
    set_numeric(values, "control_state", msg.control_state);
    set_value(values, "control_state_name", uav_control_fsm_name(msg.control_state));
    set_control_cmd(values, "last_cmd.", msg.last_cmd);
    set_value(values,
              "last_cmd.cmd_source_name",
              uav_control_cmd_source_name(msg.last_cmd.cmd_source));
    set_value(values, "last_cmd.control_cmd_name", uav_control_cmd_name(msg.last_cmd.control_cmd));
    set_value(values, "last_cmd.yaw_mode_name", uav_yaw_mode_name(msg.last_cmd.yaw_mode));
    set_odometry(values, "self_odom.", msg.self_odom);
    set_value(values, "odometry_lost", monitor_fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", monitor_fmt_bool(msg.odometry_valid));
    set_numeric(values, "controller_output_type", msg.controller_output_type);
    set_value(values,
              "controller_output_type_name",
              uav_controller_output_type_name(msg.controller_output_type));
    set_position_target(values, "position_target.", msg.position_target);
    set_value(values,
              "position_target.coordinate_frame_name",
              position_target_frame_name(msg.position_target.coordinate_frame));
    set_attitude_target(values, "attitude_target.", msg.attitude_target);
}

void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "connected", monitor_fmt_bool(msg.connected));
    set_value(values, "rc_available", monitor_fmt_bool(msg.rc_available));
    set_value(values, "armed", monitor_fmt_bool(msg.armed));
    set_numeric(values, "flight_mode", msg.flight_mode);
    set_numeric(values, "system_status", msg.system_status);
    set_numeric(values, "landed_state", msg.landed_state);
    set_value(values, "landed_state_name", px4_landed_state_name(msg.landed_state));
    set_float(values, "battery_voltage_v", msg.battery_voltage_v);
    set_float(values, "battery_current_a", msg.battery_current_a);
    set_float(values, "battery_percentage", msg.battery_percentage);
    set_numeric(values, "fcu_load", msg.fcu_load);
    set_pose(values, "external_pose.", msg.external_pose);
    set_twist(values, "external_velocity.", msg.external_velocity);
    set_pose(values, "local_pose.", msg.local_pose);
    set_twist(values, "local_velocity.", msg.local_velocity);
    set_numeric(values, "setpoint_coordinate_frame", msg.setpoint_coordinate_frame);
    set_numeric(values, "setpoint_local_type_mask", msg.setpoint_local_type_mask);
    set_vec3(values, "pos_setpoint_m", msg.pos_setpoint_m);
    set_vec3(values, "vel_setpoint_mps", msg.vel_setpoint_mps);
    set_vec3(values, "acc_setpoint_mps2", msg.acc_setpoint_mps2);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint_rad);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint_radps);
    set_numeric(values, "setpoint_att_type_mask", msg.setpoint_att_type_mask);
    set_quat(values, "orientation_setpoint", msg.orientation_setpoint);
    set_vec3(values, "body_rate_setpoint_radps", msg.body_rate_setpoint_radps);
    set_float(values, "thrust_setpoint", msg.thrust_setpoint);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude_deg);
    set_float(values, "longitude_deg", msg.longitude_deg);
    set_float(values, "altitude_m", msg.altitude_m);
    set_float(values, "latitude_raw_deg", msg.latitude_raw_deg);
    set_float(values, "longitude_raw_deg", msg.longitude_raw_deg);
    set_float(values, "altitude_amsl_m", msg.altitude_amsl_m);
}

void fill_command_execution_status_from_yunlink(
    const yunlink::CommandExecutionStatusSnapshot& msg,
    std::unordered_map<std::string, std::string>& values) {
    set_command_execution_status(values, "", msg);
}

void fill_sunray_runtime_diagnostic_from_yunlink(
    const yunlink::SunrayRuntimeDiagnosticSnapshot& msg,
    std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "agent_key", msg.agent_key);
    set_numeric(values, "stale_timeout_ms", msg.stale_timeout_ms);
    set_value(values, "runtime_started", monitor_fmt_bool(msg.runtime_started));
    set_value(values, "peer_ready", monitor_fmt_bool(msg.peer_ready));
    set_value(values, "session_state", msg.session_state);
    set_value(values, "last_connect_error", msg.last_connect_error);
    set_value(values, "last_session_error", msg.last_session_error);
    set_value(values, "last_publish_error", msg.last_publish_error);
    set_numeric(values, "last_error_age_ms", msg.last_error_age_ms);
    set_numeric(values, "connect_attempt_count", msg.connect_attempt_count);
    set_numeric(values, "session_lost_count", msg.session_lost_count);
    set_numeric(values, "ros_to_yunlink_publish_count", msg.ros_to_yunlink_publish_count);
    set_numeric(values, "ros_to_yunlink_fail_count", msg.ros_to_yunlink_fail_count);
    set_numeric(values, "yunlink_to_ros_command_count", msg.yunlink_to_ros_command_count);
    set_numeric(values, "yunlink_to_ros_publish_count", msg.yunlink_to_ros_publish_count);
    set_numeric(values, "yunlink_to_ros_fail_count", msg.yunlink_to_ros_fail_count);
    set_value(values, "last_fail_direction", msg.last_fail_direction);
    set_value(values, "last_fail_key", msg.last_fail_key);
    set_numeric(values, "last_fail_error_code", msg.last_fail_error_code);
    set_value(values, "last_fail_detail", msg.last_fail_detail);
    set_topic_diagnostic(values, "external_odom.", msg.external_odom);
    set_topic_diagnostic(values, "odom_state.", msg.odom_state);
    set_topic_diagnostic(values, "local_odom.", msg.local_odom);
    set_topic_diagnostic(values, "global_odom.", msg.global_odom);
    set_topic_diagnostic(values, "uav_control_cmd.", msg.uav_control_cmd);
    set_topic_diagnostic(values, "uav_control_state.", msg.uav_control_state);
    set_topic_diagnostic(values, "px4_state.", msg.px4_state);
    set_value(values, "worst_level", msg.worst_level);
    set_value(values, "summary", msg.summary);
}
