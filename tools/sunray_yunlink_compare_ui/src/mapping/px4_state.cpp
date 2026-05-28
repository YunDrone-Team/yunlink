#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_px4_state_from_ros(const sunray_msgs::Px4State& msg,
                             std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_numeric(values, "flight_mode", msg.flight_mode);
    set_value(values, "flight_mode_name", fmt_num(msg.flight_mode));
    set_numeric(values, "system_status", msg.system_status);
    set_numeric(values, "landed_state", msg.landed_state);
    set_float(values, "battery_voltage_v", msg.battery_voltage_v);
    set_float(values, "battery_current_a", msg.battery_current_a);
    set_float(values, "battery_percentage", msg.battery_percentage);
    set_float(values, "local_position_m.x", msg.local_pose.position.x);
    set_float(values, "local_position_m.y", msg.local_pose.position.y);
    set_float(values, "local_position_m.z", msg.local_pose.position.z);
    set_float(values, "local_velocity_mps.x", msg.local_velocity.linear.x);
    set_float(values, "local_velocity_mps.y", msg.local_velocity.linear.y);
    set_float(values, "local_velocity_mps.z", msg.local_velocity.linear.z);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude);
    set_float(values, "longitude_deg", msg.longitude);
    set_float(values, "altitude_m", msg.altitude);
}

void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_numeric(values, "flight_mode", msg.flight_mode);
    set_value(values, "flight_mode_name", msg.flight_mode_name);
    set_numeric(values, "system_status", msg.system_status);
    set_numeric(values, "landed_state", msg.landed_state);
    set_float(values, "battery_voltage_v", msg.battery_voltage_v);
    set_float(values, "battery_current_a", msg.battery_current_a);
    set_float(values, "battery_percentage", msg.battery_percentage);
    set_float(values, "local_position_m.x", msg.local_position_m.x);
    set_float(values, "local_position_m.y", msg.local_position_m.y);
    set_float(values, "local_position_m.z", msg.local_position_m.z);
    set_float(values, "local_velocity_mps.x", msg.local_velocity_mps.x);
    set_float(values, "local_velocity_mps.y", msg.local_velocity_mps.y);
    set_float(values, "local_velocity_mps.z", msg.local_velocity_mps.z);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint_rad);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint_radps);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude_deg);
    set_float(values, "longitude_deg", msg.longitude_deg);
    set_float(values, "altitude_m", msg.altitude_m);
}
