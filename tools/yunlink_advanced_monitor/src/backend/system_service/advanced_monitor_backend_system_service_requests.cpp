#include "backend/advanced_monitor_backend.hpp"

void AdvancedMonitorBackend::bind_system_service_feedback() {
    feature_list_response_token_ =
        runtime_.system_service_subscriber().subscribe_feature_list_responses(
            [this](const yunlink::TypedMessage<yunlink::FeatureListResponse>& message) {
                on_feature_list_response(message);
            });
    feature_get_response_token_ =
        runtime_.system_service_subscriber().subscribe_feature_get_responses(
            [this](const yunlink::TypedMessage<yunlink::FeatureGetResponse>& message) {
                on_feature_get_response(message);
            });
    feature_start_response_token_ =
        runtime_.system_service_subscriber().subscribe_feature_start_responses(
            [this](const yunlink::TypedMessage<yunlink::FeatureStartResponse>& message) {
                on_feature_start_response(message);
            });
    feature_stop_response_token_ =
        runtime_.system_service_subscriber().subscribe_feature_stop_responses(
            [this](const yunlink::TypedMessage<yunlink::FeatureStopResponse>& message) {
                on_feature_stop_response(message);
            });
    log(MonitorLogLevel::kInfo, MonitorLogSource::kRuntime, "system service response 订阅器已就绪");
}

void AdvancedMonitorBackend::request_feature_list() {
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureList 未发送，session 未就绪");
        return;
    }

    yunlink::FeatureListRequest request{};
    yunlink::SystemServiceHandle handle{};
    const auto ec = runtime_.system_service_publisher().publish_feature_list_request(
        peer_id, session_id, system_service_target(), request, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kSystemService,
            "FeatureList 发送失败，ec=" + error_code_label(ec));
        return;
    }

    record_system_service_request("FeatureList", std::string(), handle);
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kSystemService,
        "已发送 FeatureList 请求，message_id=" + std::to_string(handle.message_id));
}

void AdvancedMonitorBackend::request_feature_get(const std::string& feature_name) {
    if (feature_name.empty()) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureGet 未发送，feature_name 为空");
        return;
    }

    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureGet 未发送，session 未就绪");
        return;
    }

    yunlink::FeatureGetRequest request{};
    request.feature_name = feature_name;
    yunlink::SystemServiceHandle handle{};
    const auto ec = runtime_.system_service_publisher().publish_feature_get_request(
        peer_id, session_id, system_service_target(), request, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kSystemService,
            "FeatureGet 发送失败，feature=" + feature_name + " ec=" + error_code_label(ec));
        return;
    }

    record_system_service_request("FeatureGet", feature_name, handle);
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kSystemService,
        "已发送 FeatureGet 请求，feature=" + feature_name +
            " message_id=" + std::to_string(handle.message_id));
}

void AdvancedMonitorBackend::request_feature_start(const std::string& feature_name,
                                                   const std::vector<std::string>& override_args,
                                                   bool restart_if_running,
                                                   bool start_with_terminal) {
    if (feature_name.empty()) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureStart 未发送，feature_name 为空");
        return;
    }

    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureStart 未发送，session 未就绪");
        return;
    }

    yunlink::FeatureStartRequest request{};
    request.feature_name = feature_name;
    request.override_args = override_args;
    request.restart_if_running = restart_if_running;
    request.start_with_terminal = start_with_terminal;
    yunlink::SystemServiceHandle handle{};
    const auto ec = runtime_.system_service_publisher().publish_feature_start_request(
        peer_id, session_id, system_service_target(), request, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kSystemService,
            "FeatureStart 发送失败，feature=" + feature_name + " ec=" + error_code_label(ec));
        return;
    }

    record_system_service_request("FeatureStart", feature_name, handle);
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kSystemService,
        "已发送 FeatureStart 请求，feature=" + feature_name +
            " message_id=" + std::to_string(handle.message_id));
}

void AdvancedMonitorBackend::request_feature_stop(const std::string& feature_name, bool force) {
    if (feature_name.empty()) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureStop 未发送，feature_name 为空");
        return;
    }

    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSystemService,
            "FeatureStop 未发送，session 未就绪");
        return;
    }

    yunlink::FeatureStopRequest request{};
    request.feature_name = feature_name;
    request.force = force;
    yunlink::SystemServiceHandle handle{};
    const auto ec = runtime_.system_service_publisher().publish_feature_stop_request(
        peer_id, session_id, system_service_target(), request, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kSystemService,
            "FeatureStop 发送失败，feature=" + feature_name + " ec=" + error_code_label(ec));
        return;
    }

    record_system_service_request("FeatureStop", feature_name, handle);
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kSystemService,
        "已发送 FeatureStop 请求，feature=" + feature_name +
            " message_id=" + std::to_string(handle.message_id));
}

void AdvancedMonitorBackend::record_system_service_request(
    const std::string& action,
    const std::string& feature_name,
    const yunlink::SystemServiceHandle& handle) {
    std::lock_guard<std::mutex> lock(mu_);
    MonitorSystemServiceHistoryEntry entry;
    entry.sequence = next_system_service_sequence_++;
    entry.sent_at_ms = wall_time_ms();
    entry.updated_at_ms = entry.sent_at_ms;
    entry.session_id = handle.session_id;
    entry.message_id = handle.message_id;
    entry.correlation_id = handle.correlation_id;
    entry.lifecycle = MonitorSystemServiceLifecycle::kPending;
    entry.action = action;
    entry.feature_name = feature_name;
    system_service_history_.push_back(std::move(entry));
    if (system_service_history_.size() > system_service_history_limit_) {
        system_service_history_.erase(
            system_service_history_.begin(),
            system_service_history_.begin() +
                static_cast<std::ptrdiff_t>(system_service_history_.size() -
                                            system_service_history_limit_));
    }
}
