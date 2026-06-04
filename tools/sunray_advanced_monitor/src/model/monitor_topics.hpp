#ifndef SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP
#define SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP

#include <cstdint>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

#include <ros/ros.h>

struct MonitorFieldDef {
    std::string key;
    std::string label;
};

struct MonitorTopicSnapshot {
    std::unordered_map<std::string, std::string> values;
    ros::Time msg_stamp;
    ros::Time receive_time;
    std::string note;
    uint64_t message_id{0};
    uint64_t created_at_ms{0};
    uint64_t session_id{0};
};

struct MonitorTopicState {
    std::string key;
    std::string title;
    std::string ros_topic;
    std::string yunlink_name;
    std::vector<MonitorFieldDef> rows;
    MonitorTopicSnapshot ros_latest;
    MonitorTopicSnapshot latest;
    double source_dt_ms{std::numeric_limits<double>::quiet_NaN()};
    double aligned_delay_ms{std::numeric_limits<double>::quiet_NaN()};
};

std::unordered_map<std::string, MonitorTopicState> make_default_monitor_topics();
std::vector<std::string> monitor_topic_display_order();
bool monitor_has_snapshot(const MonitorTopicSnapshot& snapshot);

#endif  // SUNRAY_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP
