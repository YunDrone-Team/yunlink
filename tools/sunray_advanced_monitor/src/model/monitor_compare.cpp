#include "model/monitor_compare.hpp"

#include <cmath>
#include <cstdlib>

namespace {

bool has_prefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

}  // namespace

bool monitor_is_numeric(const std::string& value) {
    if (value.empty() || value == "--" || value == "WAIT") {
        return false;
    }
    char* end = nullptr;
    std::strtod(value.c_str(), &end);
    return end != nullptr && *end == '\0';
}

bool monitor_equal_text(const std::string& lhs, const std::string& rhs) {
    return lhs == rhs;
}

bool monitor_equal_float(const std::string& lhs, const std::string& rhs, double eps) {
    try {
        return std::fabs(std::stod(lhs) - std::stod(rhs)) <= eps;
    } catch (...) {
        return false;
    }
}

double monitor_field_epsilon(const std::string& topic_key, const std::string& field_key) {
    if (topic_key == "local_odom") {
        if (has_prefix(field_key, "pose.position_m.") || has_prefix(field_key, "twist.linear_mps.") ||
            has_prefix(field_key, "twist.angular_radps.")) {
            return 1e-3;
        }
    }
    if (topic_key == "px4_state") {
        if (has_prefix(field_key, "external_pose.position_m.") ||
            has_prefix(field_key, "external_velocity.linear_mps.") ||
            has_prefix(field_key, "external_velocity.angular_radps.") ||
            has_prefix(field_key, "local_pose.position_m.") ||
            has_prefix(field_key, "local_velocity.linear_mps.") ||
            has_prefix(field_key, "local_velocity.angular_radps.") ||
            has_prefix(field_key, "pos_setpoint_m.") ||
            has_prefix(field_key, "vel_setpoint_mps.") ||
            has_prefix(field_key, "acc_setpoint_mps2.") ||
            has_prefix(field_key, "body_rate_setpoint_radps.") ||
            field_key == "yaw_setpoint_rad" || field_key == "yaw_rate_setpoint_radps" ||
            field_key == "thrust_setpoint") {
            return 1e-3;
        }
    }
    return 1e-4;
}

std::string monitor_compare_result(const std::string& topic_key,
                                   const std::string& field_key,
                                   const MonitorTopicSnapshot& ros_snapshot,
                                   const MonitorTopicSnapshot& yunlink_snapshot) {
    const auto ros_it = ros_snapshot.values.find(field_key);
    const auto yn_it = yunlink_snapshot.values.find(field_key);
    if (ros_it == ros_snapshot.values.end() || yn_it == yunlink_snapshot.values.end()) {
        return "WAIT";
    }

    const std::string& ros_value = ros_it->second;
    const std::string& yunlink_value = yn_it->second;
    if (monitor_is_numeric(ros_value) && monitor_is_numeric(yunlink_value)) {
        return monitor_equal_float(ros_value,
                                   yunlink_value,
                                   monitor_field_epsilon(topic_key, field_key))
                   ? "OK"
                   : "DIFF";
    }
    return monitor_equal_text(ros_value, yunlink_value) ? "OK" : "DIFF";
}
