#include "model/monitor_compare.hpp"

#include <cmath>
#include <cstdlib>
#include <limits>
#include <sstream>

namespace {

bool has_prefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool has_token(const std::string& value, const std::string& token) {
    return value.find(token) != std::string::npos;
}

bool same_source_stamp(const MonitorTopicSnapshot& lhs, const MonitorTopicSnapshot& rhs) {
    return !lhs.msg_stamp.isZero() && !rhs.msg_stamp.isZero() &&
           lhs.msg_stamp.sec == rhs.msg_stamp.sec && lhs.msg_stamp.nsec == rhs.msg_stamp.nsec;
}

constexpr double kTimingErrorWindowMs = 5.0;
constexpr double kNormalWindowMs = 50.0;
constexpr double kAttentionWindowMs = 150.0;
constexpr double kDelayAttentionMs = 80.0;
constexpr double kDelayLargeMs = 200.0;
constexpr double kValueStableUnalignedMs = 300.0;

std::string fmt_ms_compact(double value_ms) {
    std::ostringstream oss;
    oss.setf(std::ios::fixed);
    oss.precision(std::fabs(value_ms) >= 100.0 ? 0 : 1);
    oss << value_ms;
    std::string text = oss.str();
    if (text.find('.') != std::string::npos) {
        while (!text.empty() && text.back() == '0') {
            text.pop_back();
        }
        if (!text.empty() && text.back() == '.') {
            text.pop_back();
        }
    }
    return text;
}

std::string fmt_signed_ms(double value_ms) {
    return std::string(value_ms >= 0.0 ? "+" : "") + fmt_ms_compact(value_ms) + " ms";
}

double snapshot_age_sec(const MonitorTopicSnapshot& snapshot, const ros::Time& now) {
    if (snapshot.receive_time.isZero()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return (now - snapshot.receive_time).toSec();
}

MonitorCompareResult aligned_delay_result(double aligned_delay_ms) {
    if (std::isnan(aligned_delay_ms)) {
        return {};
    }
    if (aligned_delay_ms > kDelayLargeMs) {
        return {"通信延迟 " + fmt_ms_compact(aligned_delay_ms) + " ms", MonitorCompareLevel::kLargeDelay};
    }
    if (aligned_delay_ms > kDelayAttentionMs) {
        return {"通信延迟 " + fmt_ms_compact(aligned_delay_ms) + " ms", MonitorCompareLevel::kAttention};
    }
    return {};
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

double monitor_field_epsilon(const std::string& topic_key, const std::string& field_key) {
    (void)topic_key;
    if (has_token(field_key, "position_m.") || has_token(field_key, "linear_mps.") ||
        has_token(field_key, "angular_radps.") || has_token(field_key, "translation_m.") ||
        has_token(field_key, "desired_pos_m.") || has_token(field_key, "desired_vel_mps.") ||
        has_token(field_key, "desired_acc_mps2.") || has_token(field_key, "desired_jerk.") ||
        has_token(field_key, "desired_body_xy_pos_m.") ||
        has_token(field_key, "desired_body_xy_vel_mps.") ||
        has_token(field_key, "home_point_m.") || has_token(field_key, "pos_setpoint_m.") ||
        has_token(field_key, "vel_setpoint_mps.") ||
        has_token(field_key, "acc_setpoint_mps2.") ||
        has_token(field_key, "body_rate_setpoint_radps.")) {
        return 1e-3;
    }
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

bool monitor_values_equal(const std::string& topic_key,
                          const std::string& field_key,
                          const MonitorTopicSnapshot& ros_snapshot,
                          const MonitorTopicSnapshot& yunlink_snapshot) {
    const auto ros_it = ros_snapshot.values.find(field_key);
    const auto yn_it = yunlink_snapshot.values.find(field_key);
    if (ros_it == ros_snapshot.values.end() || yn_it == yunlink_snapshot.values.end()) {
        return false;
    }

    const std::string& ros_value = ros_it->second;
    const std::string& yunlink_value = yn_it->second;
    if (monitor_is_numeric(ros_value) && monitor_is_numeric(yunlink_value)) {
        return monitor_equal_float(ros_value, yunlink_value, monitor_field_epsilon(topic_key, field_key));
    }
    return monitor_equal_text(ros_value, yunlink_value);
}

MonitorCompareResult monitor_compare_result(const std::string& topic_key,
                                            const std::string& field_key,
                                            const MonitorTopicSnapshot& ros_snapshot,
                                            const MonitorTopicSnapshot& yunlink_snapshot,
                                            double source_dt_ms,
                                            double aligned_delay_ms) {
    const auto ros_it = ros_snapshot.values.find(field_key);
    const auto yn_it = yunlink_snapshot.values.find(field_key);
    if (ros_it == ros_snapshot.values.end() || yn_it == yunlink_snapshot.values.end()) {
        return {};
    }

    const double dt_ms = source_dt_ms;
    if (std::isnan(dt_ms)) {
        return {"无时间基准", MonitorCompareLevel::kNoBaseline};
    }

    const bool values_equal = monitor_values_equal(topic_key, field_key, ros_snapshot, yunlink_snapshot);
    const bool source_stamp_aligned = same_source_stamp(ros_snapshot, yunlink_snapshot);
    if (dt_ms < -kTimingErrorWindowMs) {
        return {"时序异常 " + fmt_ms_compact(dt_ms) + " ms", MonitorCompareLevel::kTimingError};
    }
    if (std::fabs(dt_ms) > kNormalWindowMs) {
        const auto delay_outcome = aligned_delay_result(aligned_delay_ms);
        if (delay_outcome.level != MonitorCompareLevel::kWait) {
            return delay_outcome;
        }
        if (values_equal && dt_ms <= kValueStableUnalignedMs) {
            return {"值一致", MonitorCompareLevel::kNormal};
        }
        if (dt_ms <= kAttentionWindowMs) {
            return {fmt_signed_ms(dt_ms), MonitorCompareLevel::kAttention};
        }
        return {fmt_signed_ms(dt_ms), MonitorCompareLevel::kAttention};
    }
    if (std::fabs(dt_ms) <= kNormalWindowMs) {
        if (!source_stamp_aligned) {
            return values_equal ? MonitorCompareResult{"值一致", MonitorCompareLevel::kNormal}
                                : MonitorCompareResult{fmt_signed_ms(dt_ms),
                                                       MonitorCompareLevel::kAttention};
        }
        if (!values_equal) {
            return {"值异常", MonitorCompareLevel::kValueError};
        }
        const auto delay_outcome = aligned_delay_result(aligned_delay_ms);
        if (delay_outcome.level != MonitorCompareLevel::kWait) {
            return delay_outcome;
        }
        if (std::fabs(dt_ms) <= kTimingErrorWindowMs) {
            return {"~0 ms", MonitorCompareLevel::kNormal};
        }
        return {"+" + fmt_ms_compact(dt_ms) + " ms", MonitorCompareLevel::kNormal};
    }
    return {fmt_signed_ms(dt_ms), MonitorCompareLevel::kAttention};
}

MonitorCompareResult monitor_stale_result(const MonitorTopicSnapshot& ros_snapshot,
                                          const MonitorTopicSnapshot& yunlink_snapshot,
                                          double timeout_sec) {
    const ros::Time now = ros::Time::now();
    const double ros_age_sec = snapshot_age_sec(ros_snapshot, now);
    const double yn_age_sec = snapshot_age_sec(yunlink_snapshot, now);
    const bool ros_stale = !std::isnan(ros_age_sec) && ros_age_sec > timeout_sec;
    const bool yn_stale = !std::isnan(yn_age_sec) && yn_age_sec > timeout_sec;
    if (ros_stale && yn_stale) {
        return {"双侧超时", MonitorCompareLevel::kStale};
    }
    if (ros_stale) {
        return {"ROS 超时 " + fmt_ms_compact(ros_age_sec) + " s", MonitorCompareLevel::kStale};
    }
    if (yn_stale) {
        return {"YunLink 超时 " + fmt_ms_compact(yn_age_sec) + " s", MonitorCompareLevel::kStale};
    }
    return {};
}
