#ifndef YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_INTERNAL_HPP
#define YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_INTERNAL_HPP

#include <cmath>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common/monitor_format.hpp"
#include "dashboard/developer_status_model.hpp"

namespace developer_status_detail {

using Row = std::pair<std::string, std::string>;

inline DeveloperStatusLevel max_level(DeveloperStatusLevel lhs, DeveloperStatusLevel rhs) {
    return static_cast<int>(lhs) >= static_cast<int>(rhs) ? lhs : rhs;
}

inline void push_row(std::vector<Row>& rows, std::string label, std::string value) {
    rows.emplace_back(std::move(label), std::move(value));
}

inline std::string topic_value(const std::unordered_map<std::string, std::string>& values,
                               const std::string& key,
                               const std::string& fallback = "--") {
    const auto it = values.find(key);
    return it == values.end() ? fallback : it->second;
}

inline bool topic_bool(const std::unordered_map<std::string, std::string>& values,
                       const std::string& key,
                       bool fallback = false) {
    bool parsed = false;
    return monitor_parse_bool(topic_value(values, key), &parsed) ? parsed : fallback;
}

inline double topic_double(const std::unordered_map<std::string, std::string>& values,
                           const std::string& key,
                           double fallback = 0.0) {
    double parsed = 0.0;
    return monitor_parse_double(topic_value(values, key), &parsed) ? parsed : fallback;
}

inline DeveloperStatusLine make_line(DeveloperStatusLevel level,
                                     std::string title,
                                     std::string detail) {
    DeveloperStatusLine line;
    line.level = level;
    line.title = std::move(title);
    line.detail = std::move(detail);
    return line;
}

inline void add_issue(std::vector<DeveloperStatusLine>& issues,
                      DeveloperStatusLevel level,
                      const std::string& title,
                      const std::string& detail) {
    issues.push_back(make_line(level, title, detail));
}

inline const MonitorTopicState*
find_topic(const std::unordered_map<std::string, MonitorTopicState>& topics,
           const std::string& key) {
    const auto it = topics.find(key);
    return it == topics.end() ? nullptr : &it->second;
}

inline std::string snapshot_age_text(const MonitorTopicState* topic, uint64_t now_ms) {
    if (topic == nullptr || !monitor_has_snapshot(topic->latest) || topic->latest.received_at_ms == 0) {
        return "WAIT";
    }
    const uint64_t age_ms = now_ms >= topic->latest.received_at_ms ? now_ms - topic->latest.received_at_ms : 0;
    return monitor_fmt_age_ms(age_ms);
}

inline DeveloperStatusLevel age_level(const MonitorTopicState* topic, uint64_t now_ms) {
    if (topic == nullptr || !monitor_has_snapshot(topic->latest) || topic->latest.received_at_ms == 0) {
        return DeveloperStatusLevel::kWarn;
    }
    const uint64_t age_ms = now_ms >= topic->latest.received_at_ms ? now_ms - topic->latest.received_at_ms : 0;
    if (age_ms > 3000) {
        return DeveloperStatusLevel::kError;
    }
    if (age_ms > 1000) {
        return DeveloperStatusLevel::kWarn;
    }
    return DeveloperStatusLevel::kOk;
}

inline DeveloperStatusLevel topic_diagnostic_level(const std::string& status) {
    if (status == "ERROR") {
        return DeveloperStatusLevel::kError;
    }
    if (status == "OK") {
        return DeveloperStatusLevel::kOk;
    }
    return DeveloperStatusLevel::kWarn;
}

inline bool should_replace_summary(DeveloperStatusLevel current_level,
                                   const std::string& current_detail,
                                   DeveloperStatusLevel candidate_level) {
    return static_cast<int>(candidate_level) > static_cast<int>(current_level) ||
           (candidate_level == current_level && (current_detail.empty() || current_detail == "idle"));
}

inline void merge_topic_diagnostic_status(DeveloperStatusLevel* level,
                                          std::string* detail,
                                          const std::string& key,
                                          const std::string& status) {
    if (level == nullptr || detail == nullptr) {
        return;
    }
    const DeveloperStatusLevel candidate_level = topic_diagnostic_level(status);
    if (candidate_level == DeveloperStatusLevel::kOk) {
        return;
    }
    if (should_replace_summary(*level, *detail, candidate_level)) {
        *detail = key + ": " + status;
    }
    *level = max_level(*level, candidate_level);
}

inline std::string euler_text(const std::unordered_map<std::string, std::string>& values,
                              const std::string& prefix) {
    const double qx = topic_double(values, prefix + "orientation.x", 0.0);
    const double qy = topic_double(values, prefix + "orientation.y", 0.0);
    const double qz = topic_double(values, prefix + "orientation.z", 0.0);
    const double qw = topic_double(values, prefix + "orientation.w", 1.0);
    const double sinr_cosp = 2.0 * (qw * qx + qy * qz);
    const double cosr_cosp = 1.0 - 2.0 * (qx * qx + qy * qy);
    const double roll = std::atan2(sinr_cosp, cosr_cosp);
    const double sinp = 2.0 * (qw * qy - qz * qx);
    const double pitch = std::abs(sinp) >= 1.0 ? std::copysign(M_PI / 2.0, sinp) : std::asin(sinp);
    const double siny_cosp = 2.0 * (qw * qz + qx * qy);
    const double cosy_cosp = 1.0 - 2.0 * (qy * qy + qz * qz);
    const double yaw = std::atan2(siny_cosp, cosy_cosp);
    return "r/p/y=" + monitor_fmt_float(roll) + " / " + monitor_fmt_float(pitch) + " / " +
           monitor_fmt_float(yaw);
}

inline std::string xyz_text(const std::unordered_map<std::string, std::string>& values,
                            const std::string& prefix) {
    return "x/y/z=" + topic_value(values, prefix + ".x") + " / " + topic_value(values, prefix + ".y") +
           " / " + topic_value(values, prefix + ".z");
}

DeveloperStatusCard build_yunlink_card(const MonitorConnectionSnapshot& connection,
                                       std::vector<DeveloperStatusLine>& issues);
DeveloperStatusCard build_px4_card(const MonitorTopicState* px4_topic,
                                   uint64_t now_ms,
                                   std::vector<DeveloperStatusLine>& issues);
DeveloperStatusCard build_localization_card(const MonitorTopicState* odom_topic,
                                            const MonitorTopicState* diagnostic_topic,
                                            uint64_t now_ms,
                                            std::vector<DeveloperStatusLine>& issues);
DeveloperStatusCard build_control_card(const MonitorTopicState* control_topic,
                                       uint64_t now_ms,
                                       std::vector<DeveloperStatusLine>& issues);
DeveloperStatusCard build_command_card(const MonitorTopicState* cmd_topic,
                                       const MonitorTopicState* diagnostic_topic,
                                       const MonitorTopicState* control_topic,
                                       const std::vector<MonitorCommandHistoryEntry>& history,
                                       uint64_t now_ms,
                                       std::vector<DeveloperStatusLine>& issues);

}  // namespace developer_status_detail

#endif  // YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_INTERNAL_HPP
