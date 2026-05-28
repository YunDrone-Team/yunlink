#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_STATE_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_STATE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include <ros/ros.h>

struct FieldView {
    std::string key;
    std::string label;
    std::string ros_value;
    std::string yunlink_value;
    std::string delta_text;
    bool comparable = false;
    bool equal = false;
};

struct SnapshotSide {
    std::unordered_map<std::string, std::string> values;
    ros::Time msg_stamp;
    ros::Time receive_time;
    std::string note;
    uint64_t message_id = 0;
    uint64_t created_at_ms = 0;
};

struct TopicState {
    std::string key;
    std::string title;
    std::string ros_topic;
    std::string yunlink_name;
    std::vector<std::string> uncovered_fields;
    std::vector<FieldView> rows;
    SnapshotSide ros;
    SnapshotSide yunlink;
    std::deque<SnapshotSide> ros_history;
    std::deque<SnapshotSide> yunlink_history;
};

struct ComparisonSelection {
    SnapshotSide ros;
    SnapshotSide yunlink;
    bool matched = false;
    bool within_align_window = true;
    double receive_dt_ms = std::numeric_limits<double>::quiet_NaN();
};

constexpr size_t kDefaultHistoryLimit = 120;
constexpr double kDefaultAlignWindowMs = 300.0;
constexpr double kDefaultFloatEpsilon = 1e-4;
constexpr double kDynamicFloatEpsilon = 1e-3;

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_TOPIC_STATE_HPP
