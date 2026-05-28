#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_px4_state_from_ros(const sunray_msgs::Px4State& msg,
                             std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
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
    set_vec3(values, "pos_setpoint_m", msg.pos_setpoint);
    set_vec3(values, "vel_setpoint_mps", msg.vel_setpoint);
    set_vec3(values, "acc_setpoint_mps2", msg.acc_setpoint);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint);
    set_numeric(values, "setpoint_att_type_mask", msg.setpoint_att_type_mask);
    set_quat(values, "orientation_setpoint", msg.orientation_setpoint);
    set_vec3(values, "body_rate_setpoint_radps", msg.body_rate_setpoint);
    set_float(values, "thrust_setpoint", msg.thrust_setpoint);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude);
    set_float(values, "longitude_deg", msg.longitude);
    set_float(values, "altitude_m", msg.altitude);
    set_float(values, "latitude_raw_deg", msg.latitude_raw);
    set_float(values, "longitude_raw_deg", msg.longitude_raw);
    set_float(values, "altitude_amsl_m", msg.altitude_amsl);
}

void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
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
