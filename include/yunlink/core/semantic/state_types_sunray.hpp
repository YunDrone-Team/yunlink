#ifndef YUNLINK_CORE_SEMANTIC_STATE_TYPES_SUNRAY_HPP
#define YUNLINK_CORE_SEMANTIC_STATE_TYPES_SUNRAY_HPP

#include <cstdint>
#include <string>

#include "yunlink/core/semantic/state_types.hpp"

namespace yunlink {

struct SunrayTopicDiagnosticSnapshot {
    std::string key;
    std::string topic;
    bool configured = false;
    bool has_message = false;
    uint32_t publisher_count = 0;
    uint64_t message_count = 0;
    float hz = 0.0F;
    uint32_t age_ms = 0;
    bool stale = false;
    std::string status;
    std::string detail;
};

struct SunrayRuntimeDiagnosticSnapshot {
    HeaderSnapshot header;
    std::string agent_key;
    uint32_t stale_timeout_ms = 1000;
    SunrayTopicDiagnosticSnapshot external_odom;
    SunrayTopicDiagnosticSnapshot odom_state;
    SunrayTopicDiagnosticSnapshot local_odom;
    SunrayTopicDiagnosticSnapshot global_odom;
    SunrayTopicDiagnosticSnapshot uav_control_cmd;
    SunrayTopicDiagnosticSnapshot uav_control_state;
    SunrayTopicDiagnosticSnapshot px4_state;
    std::string worst_level;
    std::string summary;
};

enum class CommandExecutionState : uint8_t {
    kIdle = 0,
    kAccepted = 1,
    kRunning = 2,
    kWaitingPhysicalState = 3,
    kSucceeded = 4,
    kFailed = 5,
    kCancelled = 6,
    kTimeout = 7,
};

struct CommandExecutionStatusSnapshot {
    HeaderSnapshot header;
    std::string agent_name;
    uint8_t agent_id = 0;
    uint64_t session_id = 0;
    uint64_t command_message_id = 0;
    uint64_t command_correlation_id = 0;
    CommandKind command_kind = CommandKind::kUnknown;
    uint8_t execution_state = 0;
    uint8_t progress_percent = 0;
    bool active = false;
    bool terminal = false;
    bool success = false;
    uint16_t result_code = 0;
    std::string detail;
    uint8_t control_state = 0;
    uint8_t px4_landed_state = 0;
    bool ready_for_takeoff = false;
    bool ready_for_land = false;
    std::string busy_reason;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_STATE_TYPES_SUNRAY_HPP
