#include "backend/advanced_monitor_backend.hpp"

#include <algorithm>

yunlink::TargetSelector AdvancedMonitorBackend::configuration_service_target() const {
    return yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav,
                                               static_cast<uint32_t>(std::max(agent_id_, 0)));
}

void AdvancedMonitorBackend::request_config_resource_list() {
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kSystemService, "配置资源列表未发送，会话未就绪");
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!configuration_.supported || configuration_.list_pending) {
            return;
        }
        configuration_.list_pending = true;
        configuration_.last_status = "正在读取配置资源列表";
    }
    yunlink::ConfigurationServiceHandle handle{};
    const auto ec = runtime_.configuration_service_publisher().publish_resource_list_request(
        peer_id,
        session_id,
        configuration_service_target(),
        yunlink::ConfigResourceListRequest{},
        &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        std::lock_guard<std::mutex> lock(mu_);
        configuration_.list_pending = false;
        configuration_.last_status = "配置资源列表发送失败: " + error_code_label(ec);
    }
}

void AdvancedMonitorBackend::request_config_resource_describe(const std::string& resource_id) {
    std::string peer_id;
    uint64_t session_id = 0;
    if (resource_id.empty() || !snapshot_send_context(&peer_id, &session_id)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto& state = configuration_.resource_states[resource_id];
        if (state.schema_pending) {
            return;
        }
        state.schema_pending = true;
    }
    yunlink::ConfigResourceDescribeRequest request{resource_id};
    const auto ec = runtime_.configuration_service_publisher().publish_resource_describe_request(
        peer_id, session_id, configuration_service_target(), request, nullptr);
    if (ec != yunlink::ErrorCode::kOk) {
        std::lock_guard<std::mutex> lock(mu_);
        configuration_.resource_states[resource_id].schema_pending = false;
        configuration_.last_status = "配置 Schema 请求失败: " + error_code_label(ec);
    }
}

void AdvancedMonitorBackend::request_config_resource_get(const std::string& resource_id) {
    std::string peer_id;
    uint64_t session_id = 0;
    if (resource_id.empty() || !snapshot_send_context(&peer_id, &session_id)) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto& state = configuration_.resource_states[resource_id];
        if (state.snapshot_pending) {
            return;
        }
        state.snapshot_pending = true;
    }
    yunlink::ConfigResourceGetRequest request{resource_id};
    const auto ec = runtime_.configuration_service_publisher().publish_resource_get_request(
        peer_id, session_id, configuration_service_target(), request, nullptr);
    if (ec != yunlink::ErrorCode::kOk) {
        std::lock_guard<std::mutex> lock(mu_);
        configuration_.resource_states[resource_id].snapshot_pending = false;
        configuration_.last_status = "配置读取失败: " + error_code_label(ec);
    }
}

void AdvancedMonitorBackend::request_config_resource_patch(
    const std::string& resource_id,
    const std::string& expected_revision,
    const std::vector<yunlink::ConfigFieldValue>& updates,
    bool validate_only) {
    std::string peer_id;
    uint64_t session_id = 0;
    if (resource_id.empty() || expected_revision.empty() ||
        !snapshot_send_context(&peer_id, &session_id)) {
        return;
    }
    yunlink::ConfigResourcePatchRequest request;
    request.resource_id = resource_id;
    request.expected_revision = expected_revision;
    request.updates = updates;
    request.validate_only = validate_only;
    yunlink::ConfigurationServiceHandle handle{};
    const auto ec = runtime_.configuration_service_publisher().publish_resource_patch_request(
        peer_id, session_id, configuration_service_target(), request, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        std::lock_guard<std::mutex> lock(mu_);
        configuration_.last_status = "配置更新发送失败: " + error_code_label(ec);
        return;
    }
    std::lock_guard<std::mutex> lock(mu_);
    config_patch_validate_requests_[handle.message_id] = validate_only;
    configuration_.last_status = validate_only ? "正在校验配置" : "正在保存配置";
}

void AdvancedMonitorBackend::request_config_resource_apply(const std::string& resource_id,
                                                            const std::string& expected_revision) {
    std::string peer_id;
    uint64_t session_id = 0;
    if (resource_id.empty() || expected_revision.empty() ||
        !snapshot_send_context(&peer_id, &session_id)) {
        return;
    }
    yunlink::ConfigResourceApplyRequest request;
    request.resource_id = resource_id;
    request.expected_revision = expected_revision;
    const auto ec = runtime_.configuration_service_publisher().publish_resource_apply_request(
        peer_id, session_id, configuration_service_target(), request, nullptr);
    std::lock_guard<std::mutex> lock(mu_);
    configuration_.last_status = ec == yunlink::ErrorCode::kOk
                                     ? "正在应用配置"
                                     : "配置应用发送失败: " + error_code_label(ec);
}
