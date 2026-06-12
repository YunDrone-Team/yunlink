#include "dashboard/developer_status_model.hpp"

#include <algorithm>

#include "dashboard/developer_status_model_internal.hpp"

DeveloperStatusSnapshot build_developer_status_snapshot(
    const MonitorConnectionSnapshot& connection,
    const std::unordered_map<std::string, MonitorTopicState>& topics,
    const std::vector<MonitorCommandHistoryEntry>& command_history,
    uint64_t now_ms) {
    using namespace developer_status_detail;

    DeveloperStatusSnapshot snapshot;
    snapshot.yunlink = build_yunlink_card(connection, snapshot.issues);
    snapshot.px4 = build_px4_card(find_topic(topics, "px4_state"), now_ms, snapshot.issues);
    snapshot.localization = build_localization_card(find_topic(topics, "odom_state"),
                                                    find_topic(topics, "sunray_runtime_diagnostic"),
                                                    now_ms,
                                                    snapshot.issues);
    snapshot.control =
        build_control_card(find_topic(topics, "uav_control_state"), now_ms, snapshot.issues);
    snapshot.command = build_command_card(find_topic(topics, "uav_control_cmd"),
                                          find_topic(topics, "sunray_runtime_diagnostic"),
                                          find_topic(topics, "uav_control_state"),
                                          command_history,
                                          now_ms,
                                          snapshot.issues);
    std::stable_sort(snapshot.issues.begin(),
                     snapshot.issues.end(),
                     [](const DeveloperStatusLine& lhs, const DeveloperStatusLine& rhs) {
                         return static_cast<int>(lhs.level) > static_cast<int>(rhs.level);
                     });
    if (snapshot.issues.size() > 5) {
        snapshot.issues.resize(5);
    }
    return snapshot;
}

std::string developer_status_level_label(DeveloperStatusLevel level) {
    switch (level) {
    case DeveloperStatusLevel::kOk:
        return "OK";
    case DeveloperStatusLevel::kWarn:
        return "WARN";
    case DeveloperStatusLevel::kError:
        return "ERROR";
    }
    return "WARN";
}

std::string developer_status_level_color(DeveloperStatusLevel level) {
    switch (level) {
    case DeveloperStatusLevel::kOk:
        return "#1f7a3f";
    case DeveloperStatusLevel::kWarn:
        return "#9a6700";
    case DeveloperStatusLevel::kError:
        return "#b42318";
    }
    return "#9a6700";
}
