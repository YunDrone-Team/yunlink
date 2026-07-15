/** @file @brief C ABI configuration service request publishing. */

#include "../internal.hpp"

#include <cstring>

namespace {

bool valid_view(yunlink_string_view_t view) {
    return view.size == 0 || view.data != nullptr;
}

std::string copy_view(yunlink_string_view_t view) {
    return view.size == 0 ? std::string() : std::string(view.data, view.size);
}

bool to_cpp_value(const yunlink_config_value_view_t& source, yunlink::ConfigValue* target) {
    if (target == nullptr) {
        return false;
    }
    switch (source.type) {
    case YUNLINK_CONFIG_VALUE_BOOL:
        *target = yunlink::ConfigValue::from_bool(source.bool_value != 0);
        return true;
    case YUNLINK_CONFIG_VALUE_INT64:
        *target = yunlink::ConfigValue::from_int64(source.int64_value);
        return true;
    case YUNLINK_CONFIG_VALUE_DOUBLE:
        *target = yunlink::ConfigValue::from_double(source.double_value);
        return true;
    case YUNLINK_CONFIG_VALUE_STRING:
        if (!valid_view(source.string_value)) {
            return false;
        }
        *target = yunlink::ConfigValue::from_string(copy_view(source.string_value));
        return true;
    case YUNLINK_CONFIG_VALUE_STRING_LIST: {
        if (source.string_list_count > 256 ||
            (source.string_list_count != 0 && source.string_list == nullptr)) {
            return false;
        }
        std::vector<std::string> values;
        values.reserve(source.string_list_count);
        for (size_t index = 0; index < source.string_list_count; ++index) {
            if (!valid_view(source.string_list[index])) {
                return false;
            }
            values.push_back(copy_view(source.string_list[index]));
        }
        *target = yunlink::ConfigValue::from_string_list(std::move(values));
        return true;
    }
    case YUNLINK_CONFIG_VALUE_DOUBLE_LIST: {
        if (source.double_list_count > 256 ||
            (source.double_list_count != 0 && source.double_list == nullptr)) {
            return false;
        }
        std::vector<double> values(source.double_list,
                                   source.double_list + source.double_list_count);
        *target = yunlink::ConfigValue::from_double_list(std::move(values));
        return true;
    }
    default:
        return false;
    }
}

bool valid_common(yunlink_runtime_t* runtime,
                  const yunlink_peer_t* peer,
                  const yunlink_session_t* session,
                  const yunlink_target_selector_t* target) {
    return yunlink_c_abi::validate_input_runtime(runtime) && yunlink_c_abi::validate_peer(peer) &&
           yunlink_c_abi::validate_session(session) && yunlink_c_abi::validate_target(target);
}

void fill_handle(const yunlink::ConfigurationServiceHandle& source,
                 yunlink_configuration_handle_t* target) {
    if (target == nullptr) {
        return;
    }
    std::memset(target, 0, sizeof(*target));
    target->message_id = source.message_id;
    target->session_id = source.session_id;
    target->created_at_ms = source.created_at_ms;
    target->ttl_ms = source.ttl_ms;
}

template <typename Request, typename Publish>
yunlink_result_t publish_request(yunlink_runtime_t* runtime,
                                 const yunlink_peer_t* peer,
                                 const yunlink_session_t* session,
                                 const yunlink_target_selector_t* target,
                                 const Request& request,
                                 yunlink_configuration_handle_t* out_handle,
                                 Publish&& publish) {
    if (!valid_common(runtime, peer, session, target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::ConfigurationServiceHandle handle;
    const auto result = publish(runtime->runtime.configuration_service_publisher(),
                                peer->id,
                                session->session_id,
                                yunlink_c_abi::to_target_selector(*target),
                                request,
                                &handle);
    if (result == yunlink::ErrorCode::kOk) {
        fill_handle(handle, out_handle);
    }
    return yunlink_c_abi::to_result(result);
}

}  // namespace

extern "C" {

yunlink_result_t
yunlink_configuration_publish_resource_list_request(yunlink_runtime_t* runtime,
                                                    const yunlink_peer_t* peer,
                                                    const yunlink_session_t* session,
                                                    const yunlink_target_selector_t* target,
                                                    yunlink_configuration_handle_t* out_handle) {
    const yunlink::ConfigResourceListRequest request{};
    return publish_request(runtime,
                           peer,
                           session,
                           target,
                           request,
                           out_handle,
                           [](auto& publisher, const auto&... args) {
                               return publisher.publish_resource_list_request(args...);
                           });
}

yunlink_result_t yunlink_configuration_publish_resource_describe_request(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    yunlink_string_view_t resource_id,
    yunlink_configuration_handle_t* out_handle) {
    if (!valid_view(resource_id)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const yunlink::ConfigResourceDescribeRequest request{copy_view(resource_id)};
    return publish_request(runtime,
                           peer,
                           session,
                           target,
                           request,
                           out_handle,
                           [](auto& publisher, const auto&... args) {
                               return publisher.publish_resource_describe_request(args...);
                           });
}

yunlink_result_t
yunlink_configuration_publish_resource_get_request(yunlink_runtime_t* runtime,
                                                   const yunlink_peer_t* peer,
                                                   const yunlink_session_t* session,
                                                   const yunlink_target_selector_t* target,
                                                   yunlink_string_view_t resource_id,
                                                   yunlink_configuration_handle_t* out_handle) {
    if (!valid_view(resource_id)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const yunlink::ConfigResourceGetRequest request{copy_view(resource_id)};
    return publish_request(runtime,
                           peer,
                           session,
                           target,
                           request,
                           out_handle,
                           [](auto& publisher, const auto&... args) {
                               return publisher.publish_resource_get_request(args...);
                           });
}

yunlink_result_t yunlink_configuration_publish_resource_patch_request(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    yunlink_string_view_t resource_id,
    yunlink_string_view_t expected_revision,
    const yunlink_config_field_value_view_t* updates,
    size_t update_count,
    uint8_t validate_only,
    yunlink_configuration_handle_t* out_handle) {
    if (!valid_view(resource_id) || !valid_view(expected_revision) || update_count > 256 ||
        (update_count != 0 && updates == nullptr)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::ConfigResourcePatchRequest request;
    request.resource_id = copy_view(resource_id);
    request.expected_revision = copy_view(expected_revision);
    request.validate_only = validate_only != 0;
    request.updates.reserve(update_count);
    for (size_t index = 0; index < update_count; ++index) {
        if (!valid_view(updates[index].path)) {
            return YUNLINK_RESULT_INVALID_ARGUMENT;
        }
        yunlink::ConfigValue value;
        if (!to_cpp_value(updates[index].value, &value)) {
            return YUNLINK_RESULT_INVALID_ARGUMENT;
        }
        request.updates.push_back({copy_view(updates[index].path), std::move(value)});
    }
    return publish_request(runtime,
                           peer,
                           session,
                           target,
                           request,
                           out_handle,
                           [](auto& publisher, const auto&... args) {
                               return publisher.publish_resource_patch_request(args...);
                           });
}

yunlink_result_t
yunlink_configuration_publish_resource_apply_request(yunlink_runtime_t* runtime,
                                                     const yunlink_peer_t* peer,
                                                     const yunlink_session_t* session,
                                                     const yunlink_target_selector_t* target,
                                                     yunlink_string_view_t resource_id,
                                                     yunlink_string_view_t expected_revision,
                                                     yunlink_configuration_handle_t* out_handle) {
    if (!valid_view(resource_id) || !valid_view(expected_revision)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    const yunlink::ConfigResourceApplyRequest request{copy_view(resource_id),
                                                      copy_view(expected_revision)};
    return publish_request(runtime,
                           peer,
                           session,
                           target,
                           request,
                           out_handle,
                           [](auto& publisher, const auto&... args) {
                               return publisher.publish_resource_apply_request(args...);
                           });
}

}  // extern "C"
