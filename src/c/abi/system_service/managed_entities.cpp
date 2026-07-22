/** @file @brief C ABI managed-entity response subscriptions and borrowed views. */

#include "../internal.hpp"

#include <exception>
#include <utility>

namespace {

yunlink_string_view_t view_of(const std::string& value) {
    return {value.data(), value.size()};
}

yunlink_managed_entity_identity_view_t view_of(const yunlink::EndpointIdentity& value) {
    return {static_cast<uint8_t>(value.agent_type),
            value.agent_id,
            static_cast<uint8_t>(value.role),
            value.group_ids.data(),
            value.group_ids.size()};
}

void add_token(yunlink_runtime_t* runtime, size_t token) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->system_service_tokens.push_back(token);
}

template <typename Callback, typename View>
void invoke_callback(Callback callback, void* user_data, const View& view) noexcept {
    try {
        callback(user_data, &view);
    } catch (...) {
        const std::exception_ptr ignored = std::current_exception();
        (void)ignored;
    }
}

bool valid_subscription(yunlink_runtime_t* runtime, const void* callback, const size_t* out_token) {
    return yunlink_c_abi::validate_input_runtime(runtime) && callback != nullptr &&
           out_token != nullptr;
}

}  // namespace

extern "C" {

yunlink_result_t yunlink_system_service_subscribe_managed_entity_list_responses(
    yunlink_runtime_t* runtime,
    yunlink_managed_entity_list_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.system_service_subscriber().subscribe_managed_entity_list_responses(
            [callback,
             user_data](const yunlink::TypedMessage<yunlink::ManagedEntityListResponse>& message) {
                std::vector<std::vector<yunlink_string_view_t>> capabilities;
                capabilities.reserve(message.payload.entities.size());
                for (const auto& entity : message.payload.entities) {
                    auto& views = capabilities.emplace_back();
                    views.reserve(entity.capabilities.size());
                    for (const auto& capability : entity.capabilities) {
                        views.push_back(view_of(capability));
                    }
                }
                std::vector<yunlink_managed_entity_descriptor_view_t> entities;
                entities.reserve(message.payload.entities.size());
                for (size_t index = 0; index < message.payload.entities.size(); ++index) {
                    const auto& entity = message.payload.entities[index];
                    entities.push_back({view_of(entity.entity_uid),
                                        view_of(entity.identity),
                                        view_of(entity.display_name),
                                        view_of(entity.hardware_id),
                                        capabilities[index].data(),
                                        capabilities[index].size(),
                                        static_cast<uint8_t>(entity.availability)});
                }
                const auto& payload = message.payload;
                const yunlink_managed_entity_list_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(payload.success),
                    view_of(payload.message),
                    view_of(payload.endpoint_uid),
                    view_of(payload.revision),
                    view_of(payload.primary_identity),
                    entities.data(),
                    entities.size()};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_subscribe_managed_entity_directory_changed(
    yunlink_runtime_t* runtime,
    yunlink_managed_entity_directory_changed_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.system_service_subscriber().subscribe_managed_entity_directory_changed(
            [callback, user_data](
                const yunlink::TypedMessage<yunlink::ManagedEntityDirectoryChanged>& message) {
                const yunlink_managed_entity_directory_changed_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    view_of(message.payload.endpoint_uid),
                    view_of(message.payload.revision)};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_subscribe_managed_entity_attachment_responses(
    yunlink_runtime_t* runtime,
    yunlink_managed_entity_attachment_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.system_service_subscriber().subscribe_managed_entity_attachment_responses(
            [callback, user_data](
                const yunlink::TypedMessage<yunlink::ManagedEntityAttachmentResponse>& message) {
                std::vector<yunlink_managed_entity_attachment_result_view_t> results;
                results.reserve(message.payload.results.size());
                for (const auto& result : message.payload.results) {
                    results.push_back({view_of(result.entity_uid),
                                       static_cast<uint8_t>(result.accepted),
                                       view_of(result.message)});
                }
                std::vector<yunlink_string_view_t> attached;
                attached.reserve(message.payload.attached_entity_uids.size());
                for (const auto& entity_uid : message.payload.attached_entity_uids) {
                    attached.push_back(view_of(entity_uid));
                }
                const auto& payload = message.payload;
                const yunlink_managed_entity_attachment_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(payload.success),
                    view_of(payload.message),
                    view_of(payload.endpoint_uid),
                    view_of(payload.directory_revision),
                    results.data(),
                    results.size(),
                    attached.data(),
                    attached.size()};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

}  // extern "C"
