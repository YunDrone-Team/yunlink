#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_COMPARISON_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_COMPARISON_HPP

#include <deque>
#include <string>

#include "model/topic_state.hpp"

bool has_snapshot(const SnapshotSide& side);
double receive_dt_ms(const ros::Time& lhs, const ros::Time& rhs);
bool has_prefix(const std::string& value, const std::string& prefix);
bool equal_text(const std::string& lhs, const std::string& rhs);
bool equal_float(const std::string& lhs, const std::string& rhs, double eps = 1e-4);
std::string delta_float(const std::string& lhs, const std::string& rhs);
double field_epsilon(const std::string& topic_key, const std::string& field_key);
void push_snapshot_history(std::deque<SnapshotSide>& history,
                           const SnapshotSide& snapshot,
                           size_t history_limit);
ComparisonSelection make_latest_selection(const TopicState& topic);
ComparisonSelection make_aligned_selection(const TopicState& topic, double align_window_ms);

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_COMPARISON_HPP
