#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_local_odom_from_ros(const nav_msgs::Odometry& msg,
                              std::unordered_map<std::string, std::string>& values) {
    set_float(values, "position_m.x", msg.pose.pose.position.x);
    set_float(values, "position_m.y", msg.pose.pose.position.y);
    set_float(values, "position_m.z", msg.pose.pose.position.z);
    set_float(values, "orientation_x", msg.pose.pose.orientation.x);
    set_float(values, "orientation_y", msg.pose.pose.orientation.y);
    set_float(values, "orientation_z", msg.pose.pose.orientation.z);
    set_float(values, "orientation_w", msg.pose.pose.orientation.w);
    set_float(values, "linear_velocity_mps.x", msg.twist.twist.linear.x);
    set_float(values, "linear_velocity_mps.y", msg.twist.twist.linear.y);
    set_float(values, "linear_velocity_mps.z", msg.twist.twist.linear.z);
}

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_float(values, "position_m.x", msg.position_m.x);
    set_float(values, "position_m.y", msg.position_m.y);
    set_float(values, "position_m.z", msg.position_m.z);
    set_float(values, "orientation_x", msg.orientation_x);
    set_float(values, "orientation_y", msg.orientation_y);
    set_float(values, "orientation_z", msg.orientation_z);
    set_float(values, "orientation_w", msg.orientation_w);
    set_float(values, "linear_velocity_mps.x", msg.linear_velocity_mps.x);
    set_float(values, "linear_velocity_mps.y", msg.linear_velocity_mps.y);
    set_float(values, "linear_velocity_mps.z", msg.linear_velocity_mps.z);
}
