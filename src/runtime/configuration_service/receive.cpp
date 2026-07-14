/**
 * @file src/runtime/configuration_service/receive.cpp
 * @brief Runtime inbound configuration service dispatch.
 */

#include "../state/fanout.hpp"

namespace yunlink {

void Runtime::handle_configuration_service_envelope(const EnvelopeEvent& ev) {
    if (ev.envelope.qos_class != QosClass::kReliableOrdered) {
        ErrorEvent error;
        error.code = ErrorCode::kRejected;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "configuration-service-qos-requires-reliable-ordered";
        bus_.publish_error(error);
        return;
    }
    if (!ev.envelope.target.matches(config_.self_identity)) {
        return;
    }

#define YUNLINK_CONFIG_REQUEST_CASE(ID, TYPE, HANDLERS)                                            \
    case ConfigurationServiceType::ID:                                                             \
        if (!runtime_fanout_configuration_request<TYPE>(                                           \
                impl_->mu, ev, ev.envelope.payload, impl_->HANDLERS)) {                            \
            runtime_publish_semantic_decode_error(bus_, ev);                                       \
        }                                                                                          \
        return

#define YUNLINK_CONFIG_RESPONSE_CASE(ID, TYPE, HANDLERS)                                           \
    case ConfigurationServiceType::ID:                                                             \
        if (!runtime_fanout_snapshot<TYPE>(                                                        \
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->HANDLERS)) {                   \
            runtime_publish_semantic_decode_error(bus_, ev);                                       \
        }                                                                                          \
        return

    switch (static_cast<ConfigurationServiceType>(ev.envelope.message_type)) {
        YUNLINK_CONFIG_REQUEST_CASE(
            kResourceListRequest, ConfigResourceListRequest, config_resource_list_request_handlers);
        YUNLINK_CONFIG_RESPONSE_CASE(kResourceListResponse,
                                     ConfigResourceListResponse,
                                     config_resource_list_response_handlers);
        YUNLINK_CONFIG_REQUEST_CASE(kResourceDescribeRequest,
                                    ConfigResourceDescribeRequest,
                                    config_resource_describe_request_handlers);
        YUNLINK_CONFIG_RESPONSE_CASE(kResourceDescribeResponse,
                                     ConfigResourceDescribeResponse,
                                     config_resource_describe_response_handlers);
        YUNLINK_CONFIG_REQUEST_CASE(
            kResourceGetRequest, ConfigResourceGetRequest, config_resource_get_request_handlers);
        YUNLINK_CONFIG_RESPONSE_CASE(
            kResourceGetResponse, ConfigResourceGetResponse, config_resource_get_response_handlers);
        YUNLINK_CONFIG_REQUEST_CASE(kResourcePatchRequest,
                                    ConfigResourcePatchRequest,
                                    config_resource_patch_request_handlers);
        YUNLINK_CONFIG_RESPONSE_CASE(kResourcePatchResponse,
                                     ConfigResourcePatchResponse,
                                     config_resource_patch_response_handlers);
        YUNLINK_CONFIG_REQUEST_CASE(kResourceApplyRequest,
                                    ConfigResourceApplyRequest,
                                    config_resource_apply_request_handlers);
        YUNLINK_CONFIG_RESPONSE_CASE(kResourceApplyResponse,
                                     ConfigResourceApplyResponse,
                                     config_resource_apply_response_handlers);
    }

#undef YUNLINK_CONFIG_REQUEST_CASE
#undef YUNLINK_CONFIG_RESPONSE_CASE
}

}  // namespace yunlink
