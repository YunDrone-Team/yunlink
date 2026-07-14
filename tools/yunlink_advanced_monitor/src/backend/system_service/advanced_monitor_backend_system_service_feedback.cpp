#include "backend/advanced_monitor_backend.hpp"

namespace {

MonitorSystemServiceHistoryEntry*
find_system_service_entry(std::vector<MonitorSystemServiceHistoryEntry>& entries,
                          uint64_t message_id) {
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        if (it->message_id == message_id) {
            return &(*it);
        }
    }
    return nullptr;
}

}  // namespace

void AdvancedMonitorBackend::on_feature_list_response(
    const yunlink::TypedMessage<yunlink::FeatureListResponse>& message) {
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (auto* entry =
                find_system_service_entry(system_service_history_, message.envelope.correlation_id);
            entry != nullptr) {
            entry->updated_at_ms = wall_time_ms();
            entry->lifecycle = message.payload.success ? MonitorSystemServiceLifecycle::kSucceeded
                                                       : MonitorSystemServiceLifecycle::kFailed;
            entry->result_message = message.payload.message;
        }
        if (message.payload.success) {
            system_services_.feature_names = message.payload.feature_names;
        }
        system_services_.last_status =
            message.payload.success
                ? "FeatureList 已更新，共 " + std::to_string(message.payload.feature_names.size()) +
                      " 个 feature"
                : "FeatureList 失败: " + message.payload.message;
        system_services_.updated_at_ms = wall_time_ms();
    }

    log(message.payload.success ? MonitorLogLevel::kInfo : MonitorLogLevel::kWarn,
        MonitorLogSource::kSystemService,
        "FeatureList response success=" + std::string(message.payload.success ? "true" : "false") +
            " correlation_id=" + std::to_string(message.envelope.correlation_id) +
            " message=" + (message.payload.message.empty() ? std::string("--") : message.payload.message));
}

void AdvancedMonitorBackend::on_feature_get_response(
    const yunlink::TypedMessage<yunlink::FeatureGetResponse>& message) {
    std::string feature_name = message.payload.name;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (auto* entry =
                find_system_service_entry(system_service_history_, message.envelope.correlation_id);
            entry != nullptr) {
            entry->updated_at_ms = wall_time_ms();
            entry->lifecycle = message.payload.success ? MonitorSystemServiceLifecycle::kSucceeded
                                                       : MonitorSystemServiceLifecycle::kFailed;
            entry->result_message = message.payload.message;
            if (feature_name.empty()) {
                feature_name = entry->feature_name;
            }
        }
        if (!feature_name.empty()) {
            auto& detail = system_services_.feature_details[feature_name];
            detail.has_response = true;
            detail.success = message.payload.success;
            detail.message = message.payload.message;
            detail.name = message.payload.name.empty() ? feature_name : message.payload.name;
            detail.group = message.payload.group;
            detail.running = message.payload.running;
            detail.description = message.payload.description;
            detail.auto_start = message.payload.auto_start;
            detail.depends_on = message.payload.depends_on;
            detail.start_preview_units = message.payload.start_preview_units;
            detail.start_preview_commands = message.payload.start_preview_commands;
            detail.updated_at_ms = wall_time_ms();
        }
        system_services_.last_status =
            message.payload.success
                ? "FeatureGet 已更新: " +
                      (feature_name.empty() ? std::string("<unknown>") : feature_name)
                : "FeatureGet 失败: " + message.payload.message;
        system_services_.updated_at_ms = wall_time_ms();
    }

    log(message.payload.success ? MonitorLogLevel::kInfo : MonitorLogLevel::kWarn,
        MonitorLogSource::kSystemService,
        "FeatureGet response success=" + std::string(message.payload.success ? "true" : "false") +
            " feature=" + (feature_name.empty() ? std::string("<unknown>") : feature_name) +
            " correlation_id=" + std::to_string(message.envelope.correlation_id) + " message=" +
            (message.payload.message.empty() ? std::string("--") : message.payload.message));
}

void AdvancedMonitorBackend::on_feature_start_response(
    const yunlink::TypedMessage<yunlink::FeatureStartResponse>& message) {
    std::string feature_name = message.payload.feature_name;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (auto* entry =
                find_system_service_entry(system_service_history_, message.envelope.correlation_id);
            entry != nullptr) {
            entry->updated_at_ms = wall_time_ms();
            entry->lifecycle = message.payload.success ? MonitorSystemServiceLifecycle::kSucceeded
                                                       : MonitorSystemServiceLifecycle::kFailed;
            entry->result_message = message.payload.message;
            if (feature_name.empty()) {
                feature_name = entry->feature_name;
            }
        }
        if (!feature_name.empty()) {
            auto& detail = system_services_.feature_details[feature_name];
            detail.last_action = "FeatureStart";
            detail.last_action_message = message.payload.message;
            detail.last_action_correlation_id = message.envelope.correlation_id;
            detail.updated_at_ms = wall_time_ms();
        }
        system_services_.last_status =
            message.payload.success
                ? "FeatureStart 已完成: " +
                      (feature_name.empty() ? std::string("<unknown>") : feature_name)
                : "FeatureStart 失败: " + message.payload.message;
        system_services_.updated_at_ms = wall_time_ms();
    }

    log(message.payload.success ? MonitorLogLevel::kInfo : MonitorLogLevel::kWarn,
        MonitorLogSource::kSystemService,
        "FeatureStart response success=" + std::string(message.payload.success ? "true" : "false") +
            " feature=" + (feature_name.empty() ? std::string("<unknown>") : feature_name) +
            " correlation_id=" + std::to_string(message.envelope.correlation_id) + " message=" +
            (message.payload.message.empty() ? std::string("--") : message.payload.message));
}

void AdvancedMonitorBackend::on_feature_stop_response(
    const yunlink::TypedMessage<yunlink::FeatureStopResponse>& message) {
    std::string feature_name = message.payload.feature_name;
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (auto* entry =
                find_system_service_entry(system_service_history_, message.envelope.correlation_id);
            entry != nullptr) {
            entry->updated_at_ms = wall_time_ms();
            entry->lifecycle = message.payload.success ? MonitorSystemServiceLifecycle::kSucceeded
                                                       : MonitorSystemServiceLifecycle::kFailed;
            entry->result_message = message.payload.message;
            if (feature_name.empty()) {
                feature_name = entry->feature_name;
            }
        }
        if (!feature_name.empty()) {
            auto& detail = system_services_.feature_details[feature_name];
            detail.last_action = "FeatureStop";
            detail.last_action_message = message.payload.message;
            detail.last_action_correlation_id = message.envelope.correlation_id;
            detail.updated_at_ms = wall_time_ms();
        }
        system_services_.last_status =
            message.payload.success
                ? "FeatureStop 已完成: " +
                      (feature_name.empty() ? std::string("<unknown>") : feature_name)
                : "FeatureStop 失败: " + message.payload.message;
        system_services_.updated_at_ms = wall_time_ms();
    }

    log(message.payload.success ? MonitorLogLevel::kInfo : MonitorLogLevel::kWarn,
        MonitorLogSource::kSystemService,
        "FeatureStop response success=" + std::string(message.payload.success ? "true" : "false") +
            " feature=" + (feature_name.empty() ? std::string("<unknown>") : feature_name) +
            " correlation_id=" + std::to_string(message.envelope.correlation_id) + " message=" +
            (message.payload.message.empty() ? std::string("--") : message.payload.message));
}

void AdvancedMonitorBackend::refresh_system_service_timeouts(uint64_t now_ms) {
    std::vector<std::string> timed_out_lines;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& entry : system_service_history_) {
            if (entry.lifecycle != MonitorSystemServiceLifecycle::kPending) {
                continue;
            }
            if (entry.sent_at_ms != 0 && now_ms > entry.sent_at_ms + system_service_timeout_ms_) {
                entry.lifecycle = MonitorSystemServiceLifecycle::kTimeout;
                entry.updated_at_ms = now_ms;
                entry.result_message = "monitor-timeout-no-response";
                timed_out_lines.push_back("SystemService 超时: action=" + entry.action +
                                          " feature=" +
                                          (entry.feature_name.empty() ? std::string("--")
                                                                      : entry.feature_name) +
                                          " message_id=" + std::to_string(entry.message_id));
            }
        }
    }
    for (const auto& line : timed_out_lines) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kSystemService, line);
    }
}
