#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_odometry(values, "", msg);
}

void fill_odom_state_from_yunlink(const yunlink::OdomStateSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_numeric(values, "external_source", msg.external_source);
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
}

void fill_control_state_from_yunlink(const yunlink::UavControlStateSnapshot& msg,
                                     std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "agent_name", msg.agent_name);
    set_numeric(values, "agent_id", msg.agent_id);
    set_numeric(values, "controller_types", msg.controller_types);
    set_float(values, "takeoff_relative_height_m", msg.takeoff_relative_height_m);
    set_float(values, "takeoff_max_velocity_mps", msg.takeoff_max_velocity_mps);
    set_numeric(values, "land_type", msg.land_type);
    set_float(values, "land_max_velocity_mps", msg.land_max_velocity_mps);
    set_float(values, "home_point_m.x", msg.home_point_m.x);
    set_float(values, "home_point_m.y", msg.home_point_m.y);
    set_float(values, "home_point_m.z", msg.home_point_m.z);
    set_numeric(values, "control_state", msg.control_state);
    set_control_cmd(values, "last_cmd.", msg.last_cmd);
    set_odometry(values, "self_odom.", msg.self_odom);
    set_value(values, "odometry_lost", monitor_fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", monitor_fmt_bool(msg.odometry_valid));
    set_numeric(values, "controller_output_type", msg.controller_output_type);
    set_position_target(values, "position_target.", msg.position_target);
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
