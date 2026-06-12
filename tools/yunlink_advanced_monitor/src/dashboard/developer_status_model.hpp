#ifndef YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "model/command_model.hpp"
#include "model/monitor_state.hpp"
#include "model/monitor_topics.hpp"

enum class DeveloperStatusLevel {
    kOk,
    kWarn,
    kError,
};

struct DeveloperStatusLine {
    DeveloperStatusLevel level{DeveloperStatusLevel::kWarn};
    std::string title;
    std::string detail;
};

struct DeveloperStatusCard {
    DeveloperStatusLine summary;
    std::vector<std::pair<std::string, std::string>> rows;
};

struct DeveloperStatusSnapshot {
    DeveloperStatusCard yunlink;
    DeveloperStatusCard px4;
    DeveloperStatusCard localization;
    DeveloperStatusCard control;
    DeveloperStatusCard command;
    std::vector<DeveloperStatusLine> issues;
};

DeveloperStatusSnapshot build_developer_status_snapshot(
    const MonitorConnectionSnapshot& connection,
    const std::unordered_map<std::string, MonitorTopicState>& topics,
    const std::vector<MonitorCommandHistoryEntry>& command_history,
    uint64_t now_ms);

std::string developer_status_level_label(DeveloperStatusLevel level);
std::string developer_status_level_color(DeveloperStatusLevel level);

#endif  // YUNLINK_ADVANCED_MONITOR_DASHBOARD_DEVELOPER_STATUS_MODEL_HPP
