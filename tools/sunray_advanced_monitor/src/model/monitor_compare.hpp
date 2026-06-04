#ifndef SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP
#define SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP

#include <string>

#include "model/monitor_topics.hpp"

enum class MonitorCompareLevel {
    kWait,
    kNormal,
    kAttention,
    kNoBaseline,
    kStale,
    kValueError,
    kTimingError,
    kLargeDelay,
};

struct MonitorCompareResult {
    std::string text{"WAIT"};
    MonitorCompareLevel level{MonitorCompareLevel::kWait};
};

bool monitor_is_numeric(const std::string& value);
bool monitor_equal_text(const std::string& lhs, const std::string& rhs);
bool monitor_equal_float(const std::string& lhs, const std::string& rhs, double eps);
double monitor_field_epsilon(const std::string& topic_key, const std::string& field_key);
bool monitor_values_equal(const std::string& topic_key,
                          const std::string& field_key,
                          const MonitorTopicSnapshot& ros_snapshot,
                          const MonitorTopicSnapshot& yunlink_snapshot);
MonitorCompareResult monitor_compare_result(const std::string& topic_key,
                                            const std::string& field_key,
                                            const MonitorTopicSnapshot& ros_snapshot,
                                            const MonitorTopicSnapshot& yunlink_snapshot,
                                            double source_dt_ms,
                                            double aligned_delay_ms);
MonitorCompareResult monitor_stale_result(const MonitorTopicSnapshot& ros_snapshot,
                                          const MonitorTopicSnapshot& yunlink_snapshot,
                                          double timeout_sec);

#endif  // SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP
