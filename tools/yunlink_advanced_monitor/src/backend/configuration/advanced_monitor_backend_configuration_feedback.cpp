#include "backend/advanced_monitor_backend.hpp"

namespace {

bool status_ok(yunlink::ConfigServiceStatus status) {
    return status == yunlink::ConfigServiceStatus::kOk;
}

}  // namespace

void AdvancedMonitorBackend::bind_configuration_feedback() {
    auto& subscriber = runtime_.configuration_service_subscriber();
    config_list_response_token_ = subscriber.subscribe_resource_list_responses(
        [this](const yunlink::TypedMessage<yunlink::ConfigResourceListResponse>& message) {
            on_config_resource_list_response(message);
        });
    config_describe_response_token_ = subscriber.subscribe_resource_describe_responses(
        [this](const yunlink::TypedMessage<yunlink::ConfigResourceDescribeResponse>& message) {
            on_config_resource_describe_response(message);
        });
    config_get_response_token_ = subscriber.subscribe_resource_get_responses(
        [this](const yunlink::TypedMessage<yunlink::ConfigResourceGetResponse>& message) {
            on_config_resource_get_response(message);
        });
    config_patch_response_token_ = subscriber.subscribe_resource_patch_responses(
        [this](const yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse>& message) {
            on_config_resource_patch_response(message);
        });
    config_apply_response_token_ = subscriber.subscribe_resource_apply_responses(
        [this](const yunlink::TypedMessage<yunlink::ConfigResourceApplyResponse>& message) {
            on_config_resource_apply_response(message);
        });
}

void AdvancedMonitorBackend::on_config_resource_list_response(
    const yunlink::TypedMessage<yunlink::ConfigResourceListResponse>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    configuration_.list_pending = false;
    configuration_.last_status = message.payload.message;
    if (!status_ok(message.payload.status)) {
        return;
    }
    configuration_.resources = message.payload.resources;
    for (const auto& resource : message.payload.resources) {
        configuration_.resource_states[resource.id].descriptor = resource;
    }
}

void AdvancedMonitorBackend::on_config_resource_describe_response(
    const yunlink::TypedMessage<yunlink::ConfigResourceDescribeResponse>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    const std::string id = message.payload.resource.id;
    if (id.empty()) {
        configuration_.last_status = message.payload.message;
        return;
    }
    auto& state = configuration_.resource_states[id];
    state.schema_pending = false;
    if (status_ok(message.payload.status)) {
        state.descriptor = message.payload.resource;
        state.fields = message.payload.fields;
        state.has_schema = true;
    }
    configuration_.last_status = message.payload.message;
}

void AdvancedMonitorBackend::on_config_resource_get_response(
    const yunlink::TypedMessage<yunlink::ConfigResourceGetResponse>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    const std::string id = message.payload.snapshot.resource_id;
    if (!id.empty()) {
        auto& state = configuration_.resource_states[id];
        state.snapshot_pending = false;
        if (status_ok(message.payload.status)) {
            state.snapshot = message.payload.snapshot;
            state.has_snapshot = true;
        }
    }
    configuration_.last_status = message.payload.message;
}

void AdvancedMonitorBackend::on_config_resource_patch_response(
    const yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    bool validate_only = false;
    const auto pending = config_patch_validate_requests_.find(message.envelope.correlation_id);
    if (pending != config_patch_validate_requests_.end()) {
        validate_only = pending->second;
        config_patch_validate_requests_.erase(pending);
    }
    configuration_.last_patch = message.payload;
    configuration_.last_status = message.payload.message;
    const std::string id = message.payload.snapshot.resource_id;
    if (!validate_only && status_ok(message.payload.status) && !id.empty()) {
        auto& state = configuration_.resource_states[id];
        state.snapshot = message.payload.snapshot;
        state.has_snapshot = true;
    }
}

void AdvancedMonitorBackend::on_config_resource_apply_response(
    const yunlink::TypedMessage<yunlink::ConfigResourceApplyResponse>& message) {
    std::lock_guard<std::mutex> lock(mu_);
    configuration_.last_apply = message.payload;
    configuration_.last_status = message.payload.message;
    if (status_ok(message.payload.status) && message.payload.effects.reconnect_expected) {
        connection_.last_note = "配置已安排生效，等待端点重启并重新发现";
    }
}
