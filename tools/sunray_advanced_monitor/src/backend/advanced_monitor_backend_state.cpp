#include "backend/advanced_monitor_backend.hpp"

#include <utility>

#include "common/monitor_format.hpp"

void AdvancedMonitorBackend::on_authority_status(
    const yunlink::TypedMessage<yunlink::AuthorityStatus>& message) {
    const uint64_t now_ms = wall_time_ms();
    {
        std::lock_guard<std::mutex> lock(mu_);
        authority_pending_ = false;
        authority_request_at_ms_ = now_ms;
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
    log(message.payload.state == yunlink::AuthorityState::kRejected
                ? MonitorLogLevel::kWarn
                : MonitorLogLevel::kInfo,
        MonitorLogSource::kSession,
        line);
}

void AdvancedMonitorBackend::on_command_result(const yunlink::CommandResultView& message) {
    update_command_result_history(message);

    std::string line = "command_result kind=" + command_kind_label(message.payload.command_kind) +
                       " phase=" + command_phase_label(message.payload.phase) +
                       " result_code=" + std::to_string(message.payload.result_code) +
                       " progress=" + std::to_string(message.payload.progress_percent) +
                       " correlation_id=" + std::to_string(message.envelope.correlation_id) +
                       " msg_id=" + std::to_string(message.envelope.message_id);
    if (!message.payload.detail.empty()) {
        line += " detail=" + message.payload.detail;
    }
    log(message.payload.phase == yunlink::CommandPhase::kFailed ||
                message.payload.phase == yunlink::CommandPhase::kExpired
            ? MonitorLogLevel::kWarn
            : MonitorLogLevel::kInfo,
        MonitorLogSource::kSession,
        line);
}

void AdvancedMonitorBackend::log_command_handle(const std::string& action,
                                                const yunlink::CommandHandle& handle,
                                                const std::string& detail) {
    std::string line = "命令已发送: " + action + " | session_id=" +
                       std::to_string(handle.session_id) + " message_id=" +
                       std::to_string(handle.message_id) + " correlation_id=" +
                       std::to_string(handle.correlation_id) + " target=uav/" +
                       (handle.target.target_ids.empty()
                            ? std::string("?")
                            : std::to_string(handle.target.target_ids.front()));
    if (!detail.empty()) {
        line += " | " + detail;
    }
    log(MonitorLogLevel::kInfo, MonitorLogSource::kSession, line);
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
        command_history_.erase(command_history_.begin(),
                               command_history_.begin() +
                                   static_cast<std::ptrdiff_t>(command_history_.size() -
                                                               command_history_limit_));
    }
}

void AdvancedMonitorBackend::update_command_result_history(
    const yunlink::CommandResultView& message) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto it = command_history_.rbegin(); it != command_history_.rend(); ++it) {
        if (it->message_id != message.envelope.correlation_id) {
            continue;
        }
        it->updated_at_ms = wall_time_ms();
        it->result_phase = command_phase_label(message.payload.phase);
        it->result_detail = message.payload.detail;
        if (it->lifecycle != MonitorCommandLifecycle::kApplied) {
            it->lifecycle = message.payload.phase == yunlink::CommandPhase::kExpired
                                ? MonitorCommandLifecycle::kTimeout
                                : MonitorCommandLifecycle::kReceived;
        }
        return;
    }
}

void AdvancedMonitorBackend::apply_uav_control_state_history(
    const yunlink::UavControlStateSnapshot& snapshot) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto it = command_history_.rbegin(); it != command_history_.rend(); ++it) {
        if (it->lifecycle == MonitorCommandLifecycle::kApplied) {
            continue;
        }
        if (!control_cmd_matches_history(*it, snapshot.last_cmd)) {
            continue;
        }
        it->updated_at_ms = wall_time_ms();
        it->lifecycle = MonitorCommandLifecycle::kApplied;
        it->applied_detail = "last_cmd.control_cmd=" +
                             std::to_string(snapshot.last_cmd.control_cmd);
        return;
    }
}

void AdvancedMonitorBackend::refresh_command_timeouts(uint64_t now_ms) {
    std::lock_guard<std::mutex> lock(mu_);
    for (auto& entry : command_history_) {
        if (entry.lifecycle == MonitorCommandLifecycle::kApplied ||
            entry.lifecycle == MonitorCommandLifecycle::kTimeout) {
            continue;
        }
        if (entry.sent_at_ms != 0 && now_ms > entry.sent_at_ms + command_timeout_ms_) {
            entry.lifecycle = MonitorCommandLifecycle::kTimeout;
            entry.updated_at_ms = now_ms;
            if (entry.result_detail.empty()) {
                entry.result_detail = "monitor-timeout-no-apply";
            }
        }
    }
}

bool AdvancedMonitorBackend::control_cmd_matches_history(
    const MonitorCommandHistoryEntry& entry, const yunlink::UavControlCmdSnapshot& cmd) {
    if (cmd.control_cmd != control_cmd_code_for_action(entry.action)) {
        return false;
    }

    const auto get_value = [&](const char* key) -> std::string {
        const std::string token = std::string(key) + "=";
        const std::size_t pos = entry.detail.find(token);
        if (pos == std::string::npos) {
            return std::string();
        }
        const std::size_t begin = pos + token.size();
        std::size_t end = entry.detail.find(' ', begin);
        if (end == std::string::npos) {
            end = entry.detail.size();
        }
        return entry.detail.substr(begin, end - begin);
    };

    if (entry.action == monitor_cmd_name_takeoff() || entry.action == monitor_cmd_name_land() ||
        entry.action == monitor_cmd_name_return()) {
        return true;
    }
    if (entry.action == monitor_cmd_name_goto()) {
        return monitor_fmt_float(cmd.desired_pos_m.x) == get_value("x_m") &&
               monitor_fmt_float(cmd.desired_pos_m.y) == get_value("y_m") &&
               monitor_fmt_float(cmd.desired_pos_m.z) == get_value("z_m") &&
               monitor_fmt_float(cmd.desired_yaw_rad) == get_value("yaw_rad");
    }
    if (entry.action == monitor_cmd_name_velocity(false)) {
        return monitor_fmt_float(cmd.desired_vel_mps.x) == get_value("vx_mps") &&
               monitor_fmt_float(cmd.desired_vel_mps.y) == get_value("vy_mps") &&
               monitor_fmt_float(cmd.desired_vel_mps.z) == get_value("vz_mps") &&
               monitor_fmt_float(cmd.desired_yaw_rate_radps) == get_value("yaw_rate_radps");
    }
    if (entry.action == monitor_cmd_name_velocity(true)) {
        return monitor_fmt_float(cmd.desired_body_xy_vel_mps.x) == get_value("vx_mps") &&
               monitor_fmt_float(cmd.desired_body_xy_vel_mps.y) == get_value("vy_mps") &&
               monitor_fmt_float(cmd.desired_yaw_rate_radps) == get_value("yaw_rate_radps");
    }
    return false;
}

uint8_t AdvancedMonitorBackend::control_cmd_code_for_action(const std::string& action) {
    if (action == monitor_cmd_name_takeoff()) {
        return 1;
    }
    if (action == monitor_cmd_name_land()) {
        return 2;
    }
    if (action == monitor_cmd_name_return()) {
        return 3;
    }
    if (action == monitor_cmd_name_goto()) {
        return 6;
    }
    if (action == monitor_cmd_name_velocity(false)) {
        return 7;
    }
    if (action == monitor_cmd_name_velocity(true)) {
        return 10;
    }
    return 0;
}
