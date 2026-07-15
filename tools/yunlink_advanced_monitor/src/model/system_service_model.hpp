#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_SYSTEM_SERVICE_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_SYSTEM_SERVICE_MODEL_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum class MonitorSystemServiceLifecycle {
    kPending,
    kSucceeded,
    kFailed,
    kTimeout,
};

struct MonitorFeatureDetail {
    bool has_response{false};
    bool success{false};
    std::string message;
    std::string name;
    std::string group;
    bool running{false};
    std::string description;
    bool auto_start{false};
    std::vector<std::string> depends_on;
    std::vector<std::string> start_preview_units;
    std::vector<std::string> start_preview_commands;
    std::string last_action;
    std::string last_action_message;
    uint64_t last_action_correlation_id{0};
    uint64_t updated_at_ms{0};
};

struct MonitorRuntimeLog {
    std::string runtime_id;
    std::string feature_name;
    std::string title;
    std::string state;
    uint64_t started_at_ns{0};
    uint64_t finished_at_ns{0};
    bool has_exit_code{false};
    int32_t exit_code{0};
    std::string message;
    std::string chunk;
    uint64_t cursor{0};
    bool truncated{false};
    bool eof{false};
    uint64_t updated_at_ms{0};
};

struct MonitorSystemServiceState {
    uint64_t updated_at_ms{0};
    std::string last_status;
    std::vector<std::string> feature_names;
    std::unordered_map<std::string, MonitorFeatureDetail> feature_details;
    std::vector<MonitorRuntimeLog> runtime_logs;
};

struct MonitorSystemServiceHistoryEntry {
    uint64_t sequence{0};
    uint64_t sent_at_ms{0};
    uint64_t updated_at_ms{0};
    uint64_t session_id{0};
    uint64_t message_id{0};
    uint64_t correlation_id{0};
    MonitorSystemServiceLifecycle lifecycle{MonitorSystemServiceLifecycle::kPending};
    std::string action;
    std::string feature_name;
    std::string result_message;
};

std::string system_service_lifecycle_label(MonitorSystemServiceLifecycle lifecycle);

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_SYSTEM_SERVICE_MODEL_HPP
