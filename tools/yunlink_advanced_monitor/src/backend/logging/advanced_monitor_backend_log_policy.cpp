#include "backend/advanced_monitor_backend.hpp"

namespace {

std::string value_after(const std::string& line, const std::string& token) {
    const size_t pos = line.find(token);
    if (pos == std::string::npos) {
        return {};
    }
    const size_t begin = pos + token.size();
    const size_t end = line.find(' ', begin);
    return line.substr(begin, end == std::string::npos ? std::string::npos : end - begin);
}

}  // namespace

std::string AdvancedMonitorBackend::make_semantic_log_key(MonitorLogLevel level,
                                                          MonitorLogSource source,
                                                          const std::string& line) {
    if (source == MonitorLogSource::kAuthority && line.find("authority_status ") == 0) {
        return source_label(source) + "|authority_status|state=" + value_after(line, "state=") +
               "|reason_code=" + value_after(line, "reason_code=") +
               "|detail=" + value_after(line, "detail=");
    }
    if (source == MonitorLogSource::kCommand && line.find("command_result ") == 0) {
        return source_label(source) + "|command_result|kind=" + value_after(line, "kind=") +
               "|phase=" + value_after(line, "phase=") +
               "|result_code=" + value_after(line, "result_code=") +
               "|correlation_id=" + value_after(line, "correlation_id=") +
               "|msg_id=" + value_after(line, "msg_id=") +
               "|detail=" + value_after(line, "detail=");
    }
    if (source == MonitorLogSource::kCommand && line.find("command_execution_status ") == 0) {
        return source_label(source) + "|command_execution_status|kind=" + value_after(line, "kind=") +
               "|exec=" + value_after(line, "exec=") +
               "|active=" + value_after(line, "active=") +
               "|terminal=" + value_after(line, "terminal=") +
               "|msg_id=" + value_after(line, "msg_id=") +
               "|correlation_id=" + value_after(line, "correlation_id=") +
               "|busy_reason=" + value_after(line, "busy_reason=") +
               "|detail=" + value_after(line, "detail=");
    }
    return level_label(level) + "|" + source_label(source) + "|" + line;
}

bool AdvancedMonitorBackend::should_merge_repeated_log(MonitorLogSource source,
                                                       const std::string& line) {
    return source == MonitorLogSource::kAuthority || source == MonitorLogSource::kBridge ||
           (source == MonitorLogSource::kCommand &&
            (line.find("command_result ") == 0 ||
             line.find("command_execution_status ") == 0));
}

void AdvancedMonitorBackend::log_debug(MonitorLogSource, const std::string& line) {
    log(MonitorLogLevel::kInfo, MonitorLogSource::kDebug, line);
}

bool AdvancedMonitorBackend::is_command_execution_semantic_change(
    const yunlink::CommandExecutionStatusSnapshot& previous,
    const yunlink::CommandExecutionStatusSnapshot& current) {
    return previous.command_message_id != current.command_message_id ||
           previous.command_correlation_id != current.command_correlation_id ||
           previous.command_kind != current.command_kind ||
           previous.execution_state != current.execution_state ||
           previous.active != current.active || previous.terminal != current.terminal ||
           previous.success != current.success || previous.result_code != current.result_code ||
           previous.busy_reason != current.busy_reason || previous.detail != current.detail;
}

std::string AdvancedMonitorBackend::bridge_runtime_diagnostic_log_key(
    const yunlink::SunrayRuntimeDiagnosticSnapshot& payload) {
    auto topic_key = [](const yunlink::SunrayTopicDiagnosticSnapshot& item) {
        return item.key + "=" + item.status + ":" + item.detail;
    };
    return "level=" + payload.worst_level + "|summary=" + payload.summary +
           "|session=" + payload.session_state +
           "|ros_to_yunlink_fail=" + std::to_string(payload.ros_to_yunlink_fail_count) +
           "|yunlink_to_ros_fail=" + std::to_string(payload.yunlink_to_ros_fail_count) +
           "|external_odom=" + topic_key(payload.external_odom) +
           "|odom_state=" + topic_key(payload.odom_state) +
           "|local_odom=" + topic_key(payload.local_odom) +
           "|global_odom=" + topic_key(payload.global_odom) +
           "|uav_control_cmd=" + topic_key(payload.uav_control_cmd) +
           "|uav_control_state=" + topic_key(payload.uav_control_state) +
	           "|px4_state=" + topic_key(payload.px4_state);
}

std::string bridge_runtime_diagnostic_summary(
    const yunlink::SunrayRuntimeDiagnosticSnapshot& payload) {
    if (payload.summary == "bridge forwarding publish failure") {
        std::string detail = payload.last_fail_key.empty()
                                 ? "连接切换中，短暂转发失败"
                                 : "连接切换中，短暂转发失败 key=" + payload.last_fail_key;
        if (payload.last_fail_error_code != 0) {
            detail += " ec=" + std::to_string(payload.last_fail_error_code);
        }
        if (!payload.last_fail_detail.empty()) {
            detail += " detail=" + payload.last_fail_detail;
        }
        return detail;
    }
    return payload.summary;
}

void AdvancedMonitorBackend::on_sunray_runtime_diagnostic(
    const yunlink::TypedMessage<yunlink::SunrayRuntimeDiagnosticSnapshot>& message) {
    const std::string key = bridge_runtime_diagnostic_log_key(message.payload);
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (key == last_bridge_runtime_diagnostic_key_) {
            return;
        }
        last_bridge_runtime_diagnostic_key_ = key;
    }
    if (message.payload.worst_level.empty() || message.payload.worst_level == "OK") {
        log(MonitorLogLevel::kInfo,
            MonitorLogSource::kBridge,
            "bridge_diagnostic status=OK detail=" +
                bridge_runtime_diagnostic_summary(message.payload));
        return;
    }
    log(message.payload.worst_level == "ERROR" ? MonitorLogLevel::kError : MonitorLogLevel::kWarn,
        MonitorLogSource::kBridge,
        "bridge_diagnostic status=" + message.payload.worst_level +
            " detail=" + bridge_runtime_diagnostic_summary(message.payload));
}
