#include "model/comparison.hpp"

#include <cmath>

#include "model/format.hpp"

bool has_snapshot(const SnapshotSide& side) {
    return !side.values.empty();
}

double receive_dt_ms(const ros::Time& lhs, const ros::Time& rhs) {
    if (lhs.isZero() || rhs.isZero()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::fabs((lhs - rhs).toSec() * 1000.0);
}

bool has_prefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool equal_text(const std::string& lhs, const std::string& rhs) {
    return lhs == rhs;
}

bool equal_float(const std::string& lhs, const std::string& rhs, double eps) {
    try {
        const double lv = std::stod(lhs);
        const double rv = std::stod(rhs);
        if (std::isnan(lv) || std::isnan(rv)) {
            return std::isnan(lv) && std::isnan(rv);
        }
        return std::fabs(lv - rv) <= eps;
    } catch (...) {
        return false;
    }
}

std::string delta_float(const std::string& lhs, const std::string& rhs) {
    try {
        const double lv = std::stod(lhs);
        const double rv = std::stod(rhs);
        return fmt_float(rv - lv);
    } catch (...) {
        return "--";
    }
}

double field_epsilon(const std::string& topic_key, const std::string& field_key) {
    if (topic_key == "local_odom") {
        if (has_prefix(field_key, "pose.position_m.") ||
            has_prefix(field_key, "twist.linear_mps.") ||
            has_prefix(field_key, "twist.angular_radps.")) {
            return kDynamicFloatEpsilon;
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
            return kDynamicFloatEpsilon;
        }
    }
    return kDefaultFloatEpsilon;
}

void push_snapshot_history(std::deque<SnapshotSide>& history,
                           const SnapshotSide& snapshot,
                           size_t history_limit) {
    history.push_back(snapshot);
    while (history.size() > history_limit) {
        history.pop_front();
    }
}

ComparisonSelection make_latest_selection(const TopicState& topic) {
    ComparisonSelection selection;
    selection.ros = topic.ros;
    selection.yunlink = topic.yunlink;
    selection.matched = has_snapshot(selection.ros) && has_snapshot(selection.yunlink);
    if (selection.matched) {
        selection.receive_dt_ms =
            receive_dt_ms(selection.ros.receive_time, selection.yunlink.receive_time);
    }
    return selection;
}

ComparisonSelection make_aligned_selection(const TopicState& topic, double align_window_ms) {
    ComparisonSelection selection;
    if (!topic.yunlink_history.empty()) {
        selection.yunlink = topic.yunlink_history.back();
    }
    if (!has_snapshot(selection.yunlink) || topic.ros_history.empty()) {
        return selection;
    }

    const SnapshotSide* best_ros = nullptr;
    double best_dt_ms = std::numeric_limits<double>::infinity();
    for (auto it = topic.ros_history.rbegin(); it != topic.ros_history.rend(); ++it) {
        if (!has_snapshot(*it)) {
            continue;
        }
        const double current_dt_ms = receive_dt_ms(it->receive_time, selection.yunlink.receive_time);
        if (!std::isnan(current_dt_ms) && current_dt_ms < best_dt_ms) {
            best_dt_ms = current_dt_ms;
            best_ros = &(*it);
        }
    }

    if (best_ros == nullptr || best_dt_ms > align_window_ms) {
        selection.receive_dt_ms = best_dt_ms;
        selection.within_align_window = false;
        if (best_ros != nullptr) {
            selection.ros = *best_ros;
            selection.matched = true;
        }
        return selection;
    }

    selection.ros = *best_ros;
    selection.matched = true;
    selection.within_align_window = true;
    selection.receive_dt_ms = best_dt_ms;
    return selection;
}
