#include "backend/advanced_monitor_backend.hpp"

#include <chrono>

namespace {

std::string make_log_key(MonitorLogLevel level,
                         MonitorLogSource source,
                         const std::string& line) {
    return level_label(level) + "|" + source_label(source) + "|" + line;
}

}  // namespace

std::string AdvancedMonitorBackend::authority_state_label(yunlink::AuthorityState state) {
    switch (state) {
    case yunlink::AuthorityState::kObserver:
        return "OBSERVER";
    case yunlink::AuthorityState::kPendingGrant:
        return "PENDING_GRANT";
    case yunlink::AuthorityState::kController:
        return "CONTROLLER";
    case yunlink::AuthorityState::kPreempting:
        return "PREEMPTING";
    case yunlink::AuthorityState::kRevoked:
        return "REVOKED";
    case yunlink::AuthorityState::kReleased:
        return "RELEASED";
    case yunlink::AuthorityState::kRejected:
        return "REJECTED";
    }
    return "UNKNOWN";
}

std::string AdvancedMonitorBackend::command_phase_label(yunlink::CommandPhase phase) {
    switch (phase) {
    case yunlink::CommandPhase::kReceived:
        return "RECEIVED";
    case yunlink::CommandPhase::kAccepted:
        return "ACCEPTED";
    case yunlink::CommandPhase::kInProgress:
        return "IN_PROGRESS";
    case yunlink::CommandPhase::kSucceeded:
        return "SUCCEEDED";
    case yunlink::CommandPhase::kFailed:
        return "FAILED";
    case yunlink::CommandPhase::kCancelled:
        return "CANCELLED";
    case yunlink::CommandPhase::kExpired:
        return "EXPIRED";
    }
    return "UNKNOWN";
}

std::string AdvancedMonitorBackend::command_kind_label(yunlink::CommandKind kind) {
    switch (kind) {
    case yunlink::CommandKind::kTakeoff:
        return "TAKEOFF";
    case yunlink::CommandKind::kLand:
        return "LAND";
    case yunlink::CommandKind::kReturn:
        return "RETURN";
    case yunlink::CommandKind::kGoto:
        return "MOVE_POINT";
    case yunlink::CommandKind::kVelocitySetpoint:
        return "MOVE_VELOCITY";
    case yunlink::CommandKind::kTrajectoryChunk:
        return "TRAJECTORY_CHUNK";
    case yunlink::CommandKind::kFormationTask:
        return "FORMATION_TASK";
    case yunlink::CommandKind::kUnknown:
        return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::string AdvancedMonitorBackend::error_code_label(yunlink::ErrorCode code) {
    switch (code) {
    case yunlink::ErrorCode::kOk:
        return "OK";
    case yunlink::ErrorCode::kInvalidArgument:
        return "INVALID_ARGUMENT";
    case yunlink::ErrorCode::kSocketError:
        return "SOCKET_ERROR";
    case yunlink::ErrorCode::kBindError:
        return "BIND_ERROR";
    case yunlink::ErrorCode::kListenError:
        return "LISTEN_ERROR";
    case yunlink::ErrorCode::kConnectError:
        return "CONNECT_ERROR";
    case yunlink::ErrorCode::kTimeout:
        return "TIMEOUT";
    case yunlink::ErrorCode::kEncodeError:
        return "ENCODE_ERROR";
    case yunlink::ErrorCode::kDecodeError:
        return "DECODE_ERROR";
    case yunlink::ErrorCode::kChecksumMismatch:
        return "CHECKSUM_MISMATCH";
    case yunlink::ErrorCode::kInvalidHeader:
        return "INVALID_HEADER";
    case yunlink::ErrorCode::kRuntimeStopped:
        return "RUNTIME_STOPPED";
    case yunlink::ErrorCode::kProtocolMismatch:
        return "PROTOCOL_MISMATCH";
    case yunlink::ErrorCode::kUnauthorized:
        return "UNAUTHORIZED";
    case yunlink::ErrorCode::kRejected:
        return "REJECTED";
    case yunlink::ErrorCode::kInternal:
        return "INTERNAL";
    }
    return "UNKNOWN";
}

uint16_t AdvancedMonitorBackend::clamp_port(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 65535) {
        return 65535;
    }
    return static_cast<uint16_t>(value);
}

uint64_t AdvancedMonitorBackend::wall_time_ms() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

void AdvancedMonitorBackend::update_yunlink(const std::string& key,
                                            std::unordered_map<std::string, std::string>&& values,
                                            std::string note,
                                            uint64_t message_id,
                                            uint64_t created_at_ms,
                                            uint64_t session_id) {
    std::lock_guard<std::mutex> lock(mu_);
    auto it = topics_.find(key);
    if (it == topics_.end()) {
        return;
    }
    auto& snapshot = it->second.latest;
    snapshot.values = std::move(values);
    snapshot.receive_time = ros::Time::now();
    snapshot.note = std::move(note);
    snapshot.message_id = message_id;
    snapshot.created_at_ms = created_at_ms;
    snapshot.session_id = session_id;
}

void AdvancedMonitorBackend::log(MonitorLogLevel level,
                                 MonitorLogSource source,
                                 const std::string& line) {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.push_back(MonitorLogEntry{
        next_log_sequence_++,
        wall_time_ms(),
        level,
        source,
        line,
    });
    if (logs_.size() > log_limit_) {
        logs_.erase(logs_.begin());
    }

    if (level == MonitorLogLevel::kError) {
        ROS_ERROR_STREAM("[advanced_monitor][" << source_label(source) << "] " << line);
    } else if (level == MonitorLogLevel::kWarn) {
        ROS_WARN_STREAM("[advanced_monitor][" << source_label(source) << "] " << line);
    } else {
        ROS_INFO_STREAM("[advanced_monitor][" << source_label(source) << "] " << line);
    }
}

void AdvancedMonitorBackend::log_throttle(MonitorLogLevel level,
                                          MonitorLogSource source,
                                          const std::string& line) {
    const ros::Time now = ros::Time::now();
    const std::string key = make_log_key(level, source, line);
    {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = throttled_logs_.find(key);
        if (it != throttled_logs_.end() && (now - it->second).toSec() < 2.0) {
            return;
        }
        throttled_logs_[key] = now;
    }
    log(level, source, line);
}
