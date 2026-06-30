#include "backend/advanced_monitor_backend.hpp"

#include <utility>

#include "common/monitor_format.hpp"

namespace {

bool command_identity_matches(const MonitorCommandHistoryEntry& entry,
                              uint64_t message_id,
                              uint64_t correlation_id) {
    if (correlation_id != 0 && entry.correlation_id == correlation_id) {
        return true;
    }
    if (correlation_id != 0 && entry.message_id == correlation_id) {
        return true;
    }
    if (message_id != 0 && entry.message_id == message_id) {
        return true;
    }
    return message_id != 0 && entry.correlation_id == message_id;
}

}  // namespace

void AdvancedMonitorBackend::on_authority_status(
    const yunlink::TypedMessage<yunlink::AuthorityStatus>& message) {
    const uint64_t now_ms = wall_time_ms();
    bool normal_renewal = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        normal_renewal =
            connection_.authority_state == "CONTROLLER" &&
            message.payload.state == yunlink::AuthorityState::kController &&
            message.payload.reason_code == 0 &&
            (message.payload.detail.empty() || message.payload.detail == "authority-renewed");
        authority_pending_ = false;
        authority_request_at_ms_ = now_ms;
        connection_.authority_state = authority_state_label(message.payload.state);
        connection_.updated_at_ms = now_ms;
        if (message.payload.state == yunlink::AuthorityState::kController &&
            message.payload.lease_ttl_ms > 0) {
            authority_expires_at_ms_ = now_ms + message.payload.lease_ttl_ms;
        } else if (message.payload.state == yunlink::AuthorityState::kReleased ||
                   message.payload.state == yunlink::AuthorityState::kRejected ||
                   message.payload.state == yunlink::AuthorityState::kRevoked) {
            authority_expires_at_ms_ = 0;
        }
    }

    std::string line = "authority_status state=" + authority_state_label(message.payload.state) +
                       " session_id=" + std::to_string(message.payload.session_id) +
                       " ttl_ms=" + std::to_string(message.payload.lease_ttl_ms) +
                       " reason_code=" + std::to_string(message.payload.reason_code);
    if (!message.payload.detail.empty()) {
        line += " detail=" + message.payload.detail;
    }
    if (normal_renewal) {
        log_debug(MonitorLogSource::kAuthority, line);
        return;
    }
    log(message.payload.state == yunlink::AuthorityState::kRejected ? MonitorLogLevel::kWarn
                                                                    : MonitorLogLevel::kInfo,
        MonitorLogSource::kAuthority,
        line);
}

void AdvancedMonitorBackend::on_command_result(const yunlink::CommandResultView& message) {
    update_command_result_history(message);

    std::string line = "command_result kind=" + command_kind_label(message.payload.command_kind) +
                       " phase=" + command_phase_label(message.payload.phase) +
                       " result_code=" + std::to_string(message.payload.result_code) +
                       " correlation_id=" + std::to_string(message.envelope.correlation_id) +
                       " msg_id=" + std::to_string(message.envelope.message_id);
    if (!message.payload.detail.empty()) {
        line += " detail=" + message.payload.detail;
    }
    log(message.payload.phase == yunlink::CommandPhase::kFailed ||
                message.payload.phase == yunlink::CommandPhase::kExpired
            ? MonitorLogLevel::kWarn
            : MonitorLogLevel::kInfo,
        MonitorLogSource::kCommand,
        line);
}

void AdvancedMonitorBackend::on_command_execution_status(
    const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message) {
    update_command_execution_history(message);

    if (message.payload.terminal) {
        return;
    }
    std::string line =
        "command_execution_status kind=" + command_kind_label(message.payload.command_kind) +
        " exec=" + command_execution_state_label(message.payload.execution_state) +
        " active=" + std::string(message.payload.active ? "true" : "false") +
        " terminal=false msg_id=" + std::to_string(message.payload.command_message_id) +
        " correlation_id=" + std::to_string(message.payload.command_correlation_id);
    if (!message.payload.busy_reason.empty()) {
        line += " busy_reason=" + message.payload.busy_reason;
    }
    if (!message.payload.detail.empty()) {
        line += " detail=" + message.payload.detail;
    }
    log(MonitorLogLevel::kInfo, MonitorLogSource::kCommand, line);
}

void AdvancedMonitorBackend::log_command_handle(const std::string& action,
                                                const yunlink::CommandHandle& handle,
                                                const std::string& detail) {
    std::string line =
        "命令已发送: " + action + " | session_id=" + std::to_string(handle.session_id) +
        " message_id=" + std::to_string(handle.message_id) +
        " correlation_id=" + std::to_string(handle.correlation_id) + " target=uav/" +
        (handle.target.target_ids.empty() ? std::string("?")
                                          : std::to_string(handle.target.target_ids.front()));
    if (!detail.empty()) {
        line += " | " + detail;
    }
    log(MonitorLogLevel::kInfo, MonitorLogSource::kCommand, line);
}

void AdvancedMonitorBackend::record_command_sent(const std::string& action,
                                                 const std::string& detail,
                                                 const yunlink::CommandHandle& handle) {
    std::lock_guard<std::mutex> lock(mu_);
    MonitorCommandHistoryEntry entry;
    entry.sequence = next_command_sequence_++;
    entry.sent_at_ms = wall_time_ms();
    entry.updated_at_ms = entry.sent_at_ms;
    entry.session_id = handle.session_id;
    entry.message_id = handle.message_id;
    entry.correlation_id = handle.correlation_id;
    entry.lifecycle = MonitorCommandLifecycle::kSent;
    entry.action = action;
    entry.detail = detail;
    command_history_.push_back(std::move(entry));
    if (command_history_.size() > command_history_limit_) {
        command_history_.erase(
            command_history_.begin(),
            command_history_.begin() +
                static_cast<std::ptrdiff_t>(command_history_.size() - command_history_limit_));
    }
}

void AdvancedMonitorBackend::update_command_result_history(
    const yunlink::CommandResultView& message) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto it = command_history_.rbegin(); it != command_history_.rend(); ++it) {
        if (!command_identity_matches(*it,
                                      message.envelope.message_id,
                                      message.envelope.correlation_id)) {
            continue;
        }
        it->updated_at_ms = wall_time_ms();
        it->result_phase = command_phase_label(message.payload.phase);
        it->result_detail = message.payload.detail;
        it->lifecycle = command_lifecycle_from_phase(message.payload.phase);
        return;
    }
}

void AdvancedMonitorBackend::update_command_execution_history(
    const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto it = command_history_.rbegin(); it != command_history_.rend(); ++it) {
        if (!command_identity_matches(*it,
                                      message.payload.command_message_id,
                                      message.payload.command_correlation_id)) {
            continue;
        }
        if (command_lifecycle_is_terminal(it->lifecycle) && !message.payload.terminal) {
            return;
        }
        it->updated_at_ms = wall_time_ms();
        it->execution_state = command_execution_state_label(message.payload.execution_state);
        it->has_execution_status = true;
        it->ready_for_takeoff = message.payload.ready_for_takeoff;
        it->ready_for_land = message.payload.ready_for_land;
        if (!message.payload.busy_reason.empty()) {
            it->execution_detail = message.payload.busy_reason;
        } else {
            it->execution_detail = message.payload.detail;
        }
        if (message.payload.terminal) {
            it->lifecycle = message.payload.success ? MonitorCommandLifecycle::kSucceeded
                                                    : MonitorCommandLifecycle::kFailed;
        } else {
            it->lifecycle = MonitorCommandLifecycle::kActive;
        }
        return;
    }
}

void AdvancedMonitorBackend::refresh_command_timeouts(uint64_t now_ms) {
    std::vector<std::string> timed_out_lines;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& entry : command_history_) {
            if (command_lifecycle_is_terminal(entry.lifecycle)) {
                continue;
            }
            const uint64_t deadline_base_ms =
                entry.updated_at_ms != 0 ? entry.updated_at_ms : entry.sent_at_ms;
            if (deadline_base_ms != 0 && now_ms > deadline_base_ms + command_timeout_ms_) {
                entry.lifecycle = MonitorCommandLifecycle::kTimeout;
                entry.updated_at_ms = now_ms;
                if (entry.result_detail.empty()) {
                    entry.result_detail = "monitor-timeout-no-apply";
                }
                timed_out_lines.push_back("命令结果超时: action=" + entry.action +
                                          " session_id=" + std::to_string(entry.session_id) +
                                          " message_id=" + std::to_string(entry.message_id));
            }
        }
    }
    for (const auto& line : timed_out_lines) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, line);
    }
}
