/** @file @brief C ABI configuration response subscriptions and borrowed views. */

#include "views.hpp"

#include <algorithm>
#include <utility>

namespace {

using yunlink_c_abi::configuration_views::view_of;
using yunlink_c_abi::configuration_views::ViewArena;

void add_token(yunlink_runtime_t* runtime, size_t token) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->configuration_tokens.push_back(token);
}

template <typename Callback, typename View>
void invoke_callback(Callback callback, void* user_data, const View& view) noexcept {
    try {
        callback(user_data, &view);
    } catch (...) {  // NOLINT(bugprone-empty-catch): callbacks cannot unwind across the C ABI.
    }
}

bool valid_subscription(yunlink_runtime_t* runtime, const void* callback, const size_t* out_token) {
    return yunlink_c_abi::validate_input_runtime(runtime) && callback != nullptr &&
           out_token != nullptr;
}

}  // namespace

namespace yunlink_c_abi {

void unsubscribe_configuration_callbacks(yunlink_runtime_t* runtime) {
    if (runtime == nullptr) {
        return;
    }
    std::vector<size_t> tokens;
    {
        std::lock_guard<std::mutex> lock(runtime->mu);
        tokens.swap(runtime->configuration_tokens);
    }
    for (size_t token : tokens) {
        runtime->runtime.configuration_service_subscriber().unsubscribe(token);
    }
}

}  // namespace yunlink_c_abi

extern "C" {

yunlink_result_t yunlink_configuration_subscribe_resource_list_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_list_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.configuration_service_subscriber().subscribe_resource_list_responses(
            [callback,
             user_data](const yunlink::TypedMessage<yunlink::ConfigResourceListResponse>& message) {
                ViewArena arena;
                arena.resources.reserve(message.payload.resources.size());
                for (const auto& resource : message.payload.resources) {
                    arena.resources.push_back(arena.descriptor(resource));
                }
                const yunlink_config_resource_list_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(message.payload.status),
                    view_of(message.payload.message),
                    arena.resources.data(),
                    arena.resources.size()};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_configuration_subscribe_resource_describe_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_describe_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.configuration_service_subscriber().subscribe_resource_describe_responses(
            [callback, user_data](
                const yunlink::TypedMessage<yunlink::ConfigResourceDescribeResponse>& message) {
                ViewArena arena;
                arena.describe_fields(message.payload.fields);
                const yunlink_config_resource_describe_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(message.payload.status),
                    view_of(message.payload.message),
                    arena.descriptor(message.payload.resource),
                    arena.fields.data(),
                    arena.fields.size()};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_configuration_subscribe_resource_get_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_get_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.configuration_service_subscriber().subscribe_resource_get_responses(
            [callback,
             user_data](const yunlink::TypedMessage<yunlink::ConfigResourceGetResponse>& message) {
                ViewArena arena;
                const yunlink_config_resource_get_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(message.payload.status),
                    view_of(message.payload.message),
                    arena.snapshot(message.payload.snapshot)};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_configuration_subscribe_resource_patch_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_patch_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.configuration_service_subscriber().subscribe_resource_patch_responses(
            [callback, user_data](
                const yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse>& message) {
                ViewArena arena;
                arena.errors.reserve(message.payload.errors.size());
                for (const auto& error : message.payload.errors) {
                    arena.errors.push_back(
                        {view_of(error.path), view_of(error.code), view_of(error.message)});
                }
                const yunlink_config_resource_patch_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(message.payload.status),
                    view_of(message.payload.message),
                    arena.snapshot(message.payload.snapshot),
                    arena.errors.data(),
                    arena.errors.size(),
                    arena.effects(message.payload.effects)};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_configuration_subscribe_resource_apply_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_apply_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token =
        runtime->runtime.configuration_service_subscriber().subscribe_resource_apply_responses(
            [callback, user_data](
                const yunlink::TypedMessage<yunlink::ConfigResourceApplyResponse>& message) {
                ViewArena arena;
                const yunlink_config_resource_apply_response_view_t view{
                    message.envelope.session_id,
                    message.envelope.message_id,
                    message.envelope.correlation_id,
                    static_cast<uint8_t>(message.payload.status),
                    view_of(message.payload.message),
                    view_of(message.payload.applied_revision),
                    static_cast<uint8_t>(message.payload.outcome),
                    arena.effects(message.payload.effects)};
                invoke_callback(callback, user_data, view);
            });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_configuration_unsubscribe(yunlink_runtime_t* runtime, size_t token) {
    if (!yunlink_c_abi::validate_input_runtime(runtime) || token == 0) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    {
        std::lock_guard<std::mutex> lock(runtime->mu);
        const auto iterator = std::find(
            runtime->configuration_tokens.begin(), runtime->configuration_tokens.end(), token);
        if (iterator == runtime->configuration_tokens.end()) {
            return YUNLINK_RESULT_NOT_FOUND;
        }
        runtime->configuration_tokens.erase(iterator);
    }
    runtime->runtime.configuration_service_subscriber().unsubscribe(token);
    return YUNLINK_RESULT_OK;
}

}  // extern "C"
