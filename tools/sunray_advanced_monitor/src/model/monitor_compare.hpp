#ifndef SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP
#define SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP

#include <string>

#include "model/monitor_topics.hpp"

bool monitor_is_numeric(const std::string& value);
bool monitor_equal_text(const std::string& lhs, const std::string& rhs);
bool monitor_equal_float(const std::string& lhs, const std::string& rhs, double eps);
double monitor_field_epsilon(const std::string& topic_key, const std::string& field_key);
std::string monitor_compare_result(const std::string& topic_key,
                                   const std::string& field_key,
                                   const MonitorTopicSnapshot& ros_snapshot,
                                   const MonitorTopicSnapshot& yunlink_snapshot);

#endif  // SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_COMPARE_HPP
