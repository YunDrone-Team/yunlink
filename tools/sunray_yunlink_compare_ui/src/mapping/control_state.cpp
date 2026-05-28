#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_control_state_from_ros(const sunray_msgs::UAVControlState& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "agent_name", msg.agent_name);
    set_numeric(values, "agent_id", msg.agent_id);
    set_numeric(values, "controller_types", msg.controller_types);
    set_float(values, "takeoff_relative_height_m", msg.takeoff_relative_height);
    set_float(values, "takeoff_max_velocity_mps", msg.takeoff_max_velocity);
    set_numeric(values, "land_type", msg.land_type);
    set_float(values, "land_max_velocity_mps", msg.land_max_velocity);
    set_float(values, "home_point_m.x", msg.home_point.x);
    set_float(values, "home_point_m.y", msg.home_point.y);
    set_float(values, "home_point_m.z", msg.home_point.z);
    set_numeric(values, "control_state", msg.control_state);
    set_control_cmd(values, "last_cmd.", msg.last_cmd);
    set_odometry(values, "self_odom.", msg.self_odom);
    set_value(values, "odometry_lost", fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_numeric(values, "controller_output_type", msg.controller_output_type);
    set_position_target(values, "position_target.", msg.position_target);
    set_attitude_target(values, "attitude_target.", msg.attitude_target);
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
    set_value(values, "odometry_lost", fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_numeric(values, "controller_output_type", msg.controller_output_type);
    set_position_target(values, "position_target.", msg.position_target);
    set_attitude_target(values, "attitude_target.", msg.attitude_target);
}
