/**
 * @file src/c/abi/commands/system_service.cpp
 * @brief C ABI system service request functions.
 */

#include "../internal.hpp"

#include <cmath>

using namespace yunlink_c_abi;

extern "C" {

yunlink_result_t
yunlink_system_service_request_feature_list(yunlink_runtime_t* runtime,
                                            const yunlink_peer_t* peer,
                                            const yunlink_session_t* session,
                                            const yunlink_target_selector_t* target,
                                            yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    yunlink::FeatureListRequest request{};
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_feature_list_request(
            peer->id, session->session_id, to_target_selector(*target), request, &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_request_feature_get(yunlink_runtime_t* runtime,
                                                            const yunlink_peer_t* peer,
                                                            const yunlink_session_t* session,
                                                            const yunlink_target_selector_t* target,
                                                            const char* feature_name,
                                                            yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || feature_name == nullptr || feature_name[0] == '\0') {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    yunlink::FeatureGetRequest request{};
    request.feature_name = feature_name;
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_feature_get_request(
            peer->id, session->session_id, to_target_selector(*target), request, &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t
yunlink_system_service_request_feature_start(yunlink_runtime_t* runtime,
                                             const yunlink_peer_t* peer,
                                             const yunlink_session_t* session,
                                             const yunlink_target_selector_t* target,
                                             const char* feature_name,
                                             const char* const* override_args,
                                             size_t override_arg_count,
                                             uint8_t restart_if_running,
                                             uint8_t start_with_terminal,
                                             yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || feature_name == nullptr || feature_name[0] == '\0' ||
        (override_arg_count != 0 && override_args == nullptr)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }

    yunlink::FeatureStartRequest request{};
    request.feature_name = feature_name;
    request.restart_if_running = restart_if_running != 0;
    request.start_with_terminal = start_with_terminal != 0;
    request.override_args.reserve(override_arg_count);
    for (size_t index = 0; index < override_arg_count; ++index) {
        if (override_args[index] == nullptr) {
            return YUNLINK_RESULT_INVALID_ARGUMENT;
        }
        request.override_args.emplace_back(override_args[index]);
    }

    yunlink::SystemServiceHandle handle{};
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_feature_start_request(
            peer->id, session->session_id, to_target_selector(*target), request, &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t
yunlink_system_service_request_runtime_log_list(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_runtime_log_list_request(
            peer->id, session->session_id, to_target_selector(*target), {}, &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t
yunlink_system_service_request_runtime_log_read(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                yunlink_string_view_t runtime_id,
                                                uint64_t cursor,
                                                uint32_t max_bytes,
                                                yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || runtime_id.data == nullptr || runtime_id.size == 0 ||
        max_bytes == 0) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    yunlink::RuntimeLogReadRequest request{};
    request.runtime_id.assign(runtime_id.data, runtime_id.size);
    request.cursor = cursor;
    request.max_bytes = max_bytes;
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_runtime_log_read_request(
            peer->id, session->session_id, to_target_selector(*target), request, &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_request_managed_entity_list(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    const auto result =
        to_result(runtime->runtime.system_service_publisher().publish_managed_entity_list_request(
            peer->id,
            session->session_id,
            to_target_selector(*target),
            {},
            &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_request_topic_list(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SystemServiceHandle handle{};
    const auto result = to_result(runtime->runtime.system_service_publisher()
                                      .publish_topic_list_request(peer->id,
                                                                   session->session_id,
                                                                   to_target_selector(*target),
                                                                   {},
                                                                   &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_system_service_request_topic_subscription(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    const char* topic_name,
    uint8_t subscribe,
    float max_rate_hz,
    uint32_t max_payload_bytes,
    yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || topic_name == nullptr || topic_name[0] == '\0' ||
        !std::isfinite(max_rate_hz) || max_rate_hz < 0.0F ||
        max_payload_bytes > YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::TopicSubscriptionRequest request{};
    request.topic_name = topic_name;
    request.subscribe = subscribe != 0;
    request.max_rate_hz = max_rate_hz;
    request.max_payload_bytes = max_payload_bytes;
    yunlink::SystemServiceHandle handle{};
    const auto result = to_result(runtime->runtime.system_service_publisher()
                                      .publish_topic_subscription_request(peer->id,
                                                                          session->session_id,
                                                                          to_target_selector(*target),
                                                                          request,
                                                                          &handle));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    if (out_handle != nullptr) {
        out_handle->session_id = handle.session_id;
        out_handle->message_id = handle.message_id;
        out_handle->correlation_id = handle.correlation_id;
        out_handle->target = to_c_target_selector(handle.target);
    }
    return YUNLINK_RESULT_OK;
}

}  // extern "C"
