#include "backend/advanced_monitor_backend.hpp"

#include <utility>

#include "mapping/value_map.hpp"

void AdvancedMonitorBackend::bind_yunlink_subscribers() {
    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_local_odom(
        [this](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_local_odom_from_yunlink(message.payload, values);
            update_yunlink("local_odom",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_odom_state(
        [this](const yunlink::TypedMessage<yunlink::OdomStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_odom_state_from_yunlink(message.payload, values);
            update_yunlink("odom_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_uav_control_cmd(
        [this](const yunlink::TypedMessage<yunlink::UavControlCmdSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_control_cmd_from_yunlink(message.payload, values);
            update_yunlink("uav_control_cmd",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_uav_control_state(
        [this](const yunlink::TypedMessage<yunlink::UavControlStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_control_state_from_yunlink(message.payload, values);
            update_yunlink("uav_control_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_command_execution_status(
        [this](const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_command_execution_status_from_yunlink(message.payload, values);
            update_yunlink("command_execution_status",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
            on_command_execution_status(message);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_px4_state(
        [this](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_px4_state_from_yunlink(message.payload, values);
            update_yunlink("px4_state",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
        }));

    state_sub_tokens_.push_back(runtime_.state_subscriber().subscribe_sunray_runtime_diagnostic(
        [this](const yunlink::TypedMessage<yunlink::SunrayRuntimeDiagnosticSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_sunray_runtime_diagnostic_from_yunlink(message.payload, values);
            update_yunlink("sunray_runtime_diagnostic",
                           std::move(values),
                           "session=" + std::to_string(message.envelope.session_id) +
                               " msg_id=" + std::to_string(message.envelope.message_id),
                           message.payload.header.stamp_ns,
                           message.envelope.message_id,
                           message.envelope.created_at_ms,
                           message.envelope.session_id);
            on_sunray_runtime_diagnostic(message);
        }));

    log(MonitorLogLevel::kInfo, MonitorLogSource::kRuntime, "状态快照订阅器已就绪");
}

void AdvancedMonitorBackend::bind_command_feedback() {
    authority_status_token_ = runtime_.event_subscriber().subscribe_authority_status(
        [this](const yunlink::TypedMessage<yunlink::AuthorityStatus>& message) {
            on_authority_status(message);
        });
    command_result_token_ = runtime_.event_subscriber().subscribe_command_results(
        [this](const yunlink::CommandResultView& message) { on_command_result(message); });
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kRuntime,
        "authority / command result 订阅器已就绪");
}
