#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_STATE_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_STATE_HPP

#include <cstdint>
#include <string>
#include <vector>

#include <yunlink/yunlink.hpp>

enum class MonitorLogLevel {
    kInfo,
    kWarn,
    kError,
};

enum class MonitorLogSource {
    kRuntime,
    kConnection,
    kAuthority,
    kCommand,
    kSystemService,
};

struct MonitorLogEntry {
    uint64_t sequence{0};
    uint64_t timestamp_ms{0};
    MonitorLogLevel level{MonitorLogLevel::kInfo};
    MonitorLogSource source{MonitorLogSource::kRuntime};
    std::string message;
};

struct MonitorConnectionSnapshot {
    bool runtime_started{false};
    bool peer_ready{false};
    bool link_up{false};
    bool secret_configured{false};
    std::string runtime_status;
    std::string session_state;
    std::string link_state;
    std::string peer_id;
    uint64_t session_id{0};
    std::string remote_endpoint;
    std::string listen_endpoint;
    std::string udp_bind_endpoint;
    std::string udp_target_endpoint;
    std::string agent_label;
    std::string node_name;
    std::string authority_state;
    std::string last_error;
    std::string last_note;
    uint64_t updated_at_ms{0};
};

enum class MonitorCommandLifecycle {
    kSent,
    kActive,
    kSucceeded,
    kFailed,
    kCancelled,
    kTimeout,
};

struct MonitorCommandHistoryEntry {
    uint64_t sequence{0};
    uint64_t sent_at_ms{0};
    uint64_t updated_at_ms{0};
    uint64_t session_id{0};
    uint64_t message_id{0};
    uint64_t correlation_id{0};
    MonitorCommandLifecycle lifecycle{MonitorCommandLifecycle::kSent};
    std::string action;
    std::string detail;
    std::string result_phase;
    std::string result_detail;
};

std::string level_label(MonitorLogLevel level);
std::string source_label(MonitorLogSource source);
std::string transport_label(yunlink::TransportType transport);
std::string command_lifecycle_label(MonitorCommandLifecycle lifecycle);
bool command_lifecycle_is_terminal(MonitorCommandLifecycle lifecycle);
MonitorCommandLifecycle command_lifecycle_from_phase(yunlink::CommandPhase phase);

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_MONITOR_STATE_HPP
