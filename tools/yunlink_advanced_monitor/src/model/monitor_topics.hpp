#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct MonitorFieldDef {
    std::string key;
    std::string label;
};

struct MonitorTopicSnapshot {
    std::unordered_map<std::string, std::string> values;
    uint64_t source_stamp_ns{0};
    uint64_t received_at_ms{0};
    std::string note;
    uint64_t message_id{0};
    uint64_t created_at_ms{0};
    uint64_t session_id{0};
};

struct MonitorTopicState {
    std::string key;
    std::string title;
    std::string yunlink_name;
    std::vector<MonitorFieldDef> rows;
    MonitorTopicSnapshot latest;
};

std::unordered_map<std::string, MonitorTopicState> make_default_monitor_topics();
std::vector<std::string> monitor_topic_display_order();
bool monitor_has_snapshot(const MonitorTopicSnapshot& snapshot);

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_TOPICS_HPP
