/** @file @brief C ABI runtime-log response subscriptions and borrowed views. */

#include "../internal.hpp"

#include <algorithm>
#include <utility>

namespace {

yunlink_string_view_t view_of(const std::string& value) {
    return {value.data(), value.size()};
}

yunlink_runtime_log_summary_view_t view_of(const yunlink::RuntimeLogSummary& value) {
    return {view_of(value.runtime_id), view_of(value.feature_name), view_of(value.title),
            view_of(value.state),      value.started_at_ns,       value.finished_at_ns,
            static_cast<uint8_t>(value.has_exit_code), value.exit_code, view_of(value.message)};
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
    }
}

bool valid_subscription(yunlink_runtime_t* runtime, const void* callback, const size_t* out_token) {
    return yunlink_c_abi::validate_input_runtime(runtime) && callback != nullptr &&
           out_token != nullptr;
}

}  // namespace

namespace yunlink_c_abi {

void unsubscribe_system_service_callbacks(yunlink_runtime_t* runtime) {
    if (runtime == nullptr) {
        return;
    }
    std::vector<size_t> tokens;
    {
        std::lock_guard<std::mutex> lock(runtime->mu);
        tokens.swap(runtime->system_service_tokens);
    }
    for (size_t token : tokens) {
        runtime->runtime.system_service_subscriber().unsubscribe(token);
    }
}

}  // namespace yunlink_c_abi

extern "C" {

yunlink_result_t yunlink_system_service_subscribe_runtime_log_list_responses(
    yunlink_runtime_t* runtime,
    yunlink_runtime_log_list_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token = runtime->runtime.system_service_subscriber()
                             .subscribe_runtime_log_list_responses(
                                 [callback, user_data](
                                     const yunlink::TypedMessage<yunlink::RuntimeLogListResponse>&
                                         message) {
                                     std::vector<yunlink_runtime_log_summary_view_t> runtimes;
                                     runtimes.reserve(message.payload.runtimes.size());
                                     for (const auto& runtime : message.payload.runtimes) {
                                         runtimes.push_back(view_of(runtime));
                                     }
                                     const yunlink_runtime_log_list_response_view_t view{
                                         message.envelope.session_id,
                                         message.envelope.message_id,
                                         message.envelope.correlation_id,
                                         static_cast<uint8_t>(message.payload.success),
                                         view_of(message.payload.message),
                                         runtimes.data(),
                                         runtimes.size()};
                                     invoke_callback(callback, user_data, view);
                                 });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_subscribe_runtime_log_read_responses(
    yunlink_runtime_t* runtime,
    yunlink_runtime_log_read_response_callback_t callback,
    void* user_data,
    size_t* out_token) {
    if (!valid_subscription(runtime, reinterpret_cast<const void*>(callback), out_token)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const size_t token = runtime->runtime.system_service_subscriber()
                             .subscribe_runtime_log_read_responses(
                                 [callback, user_data](
                                     const yunlink::TypedMessage<yunlink::RuntimeLogReadResponse>&
                                         message) {
                                     const auto& payload = message.payload;
                                     const yunlink_runtime_log_read_response_view_t view{
                                         message.envelope.session_id,
                                         message.envelope.message_id,
                                         message.envelope.correlation_id,
                                         static_cast<uint8_t>(payload.success),
                                         view_of(payload.message),
                                         view_of(payload.runtime_id),
                                         view_of(payload.chunk),
                                         payload.next_cursor,
                                         static_cast<uint8_t>(payload.truncated),
                                         static_cast<uint8_t>(payload.eof)};
                                     invoke_callback(callback, user_data, view);
                                 });
    add_token(runtime, token);
    *out_token = token;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_unsubscribe(yunlink_runtime_t* runtime, size_t token) {
    if (!yunlink_c_abi::validate_input_runtime(runtime) || token == 0) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    {
        std::lock_guard<std::mutex> lock(runtime->mu);
        const auto found = std::find(runtime->system_service_tokens.begin(),
                                     runtime->system_service_tokens.end(), token);
        if (found == runtime->system_service_tokens.end()) {
            return YUNLINK_RESULT_NOT_FOUND;
        }
        runtime->system_service_tokens.erase(found);
    }
    runtime->runtime.system_service_subscriber().unsubscribe(token);
    return YUNLINK_RESULT_OK;
}

}  // extern "C"
