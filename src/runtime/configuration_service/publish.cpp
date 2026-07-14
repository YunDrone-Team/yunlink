/**
 * @file src/runtime/configuration_service/publish.cpp
 * @brief Runtime configuration service publishing implementation.
 */

#include "../core/internal.hpp"

namespace yunlink {
namespace {

void fill_handle(ConfigurationServiceHandle* out,
                 uint64_t session_id,
                 uint64_t message_id,
                 uint64_t created_at_ms,
                 uint32_t ttl_ms) {
    if (out == nullptr) {
        return;
    }
    out->session_id = session_id;
    out->message_id = message_id;
    out->created_at_ms = created_at_ms;
    out->ttl_ms = ttl_ms;
}

}  // namespace

ConfigurationServicePublisher::ConfigurationServicePublisher(Runtime* runtime)
    : runtime_(runtime) {}

void ConfigurationServicePublisher::bind(Runtime* runtime) {
    runtime_ = runtime;
}

ErrorCode
Runtime::publish_configuration_service_request_payload(const std::string& peer_id,
                                                       uint64_t session_id,
                                                       const TargetSelector& target,
                                                       uint16_t message_type,
                                                       const ByteBuffer& payload,
                                                       ConfigurationServiceHandle* out_handle,
                                                       uint32_t ttl_ms) {
    SessionDescriptor session{};
    if (describe_session_internal(peer_id, session_id, &session) &&
        session.state != SessionState::kActive) {
        return ErrorCode::kRejected;
    }
    SecureEnvelope envelope = make_runtime_envelope(config_.self_identity,
                                                    target,
                                                    session_id,
                                                    0,
                                                    QosClass::kReliableOrdered,
                                                    MessageFamily::kConfigurationService,
                                                    message_type,
                                                    payload,
                                                    ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;
    const ErrorCode result = send_envelope_to_peer(peer_id, envelope);
    if (result == ErrorCode::kOk) {
        fill_handle(
            out_handle, session_id, envelope.message_id, envelope.created_at_ms, envelope.ttl_ms);
    }
    return result;
}

ErrorCode Runtime::reply_configuration_service_payload(const EnvelopeEvent& inbound,
                                                       uint16_t message_type,
                                                       const ByteBuffer& payload,
                                                       uint32_t ttl_ms) {
    SecureEnvelope envelope =
        make_runtime_envelope(config_.self_identity,
                              TargetSelector::for_entity(inbound.envelope.source.agent_type,
                                                         inbound.envelope.source.agent_id),
                              inbound.envelope.session_id,
                              inbound.envelope.message_id,
                              QosClass::kReliableOrdered,
                              MessageFamily::kConfigurationService,
                              message_type,
                              payload,
                              ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = inbound.envelope.message_id;
    return reply_on_route(inbound, envelope);
}

#define YUNLINK_CONFIG_REQUEST_METHOD(METHOD, TYPE)                                                \
    ErrorCode ConfigurationServicePublisher::METHOD(const std::string& peer_id,                    \
                                                    uint64_t session_id,                           \
                                                    const TargetSelector& target,                  \
                                                    const TYPE& payload,                           \
                                                    ConfigurationServiceHandle* out_handle) {      \
        return runtime_ == nullptr ? ErrorCode::kInvalidArgument                                   \
                                   : runtime_->publish_configuration_service_request_payload(      \
                                         peer_id,                                                  \
                                         session_id,                                               \
                                         target,                                                   \
                                         MessageTraits<TYPE>::kMessageType,                        \
                                         encode_payload(payload),                                  \
                                         out_handle,                                               \
                                         3000);                                                    \
    }

YUNLINK_CONFIG_REQUEST_METHOD(publish_resource_list_request, ConfigResourceListRequest)
YUNLINK_CONFIG_REQUEST_METHOD(publish_resource_describe_request, ConfigResourceDescribeRequest)
YUNLINK_CONFIG_REQUEST_METHOD(publish_resource_get_request, ConfigResourceGetRequest)
YUNLINK_CONFIG_REQUEST_METHOD(publish_resource_patch_request, ConfigResourcePatchRequest)
YUNLINK_CONFIG_REQUEST_METHOD(publish_resource_apply_request, ConfigResourceApplyRequest)

#undef YUNLINK_CONFIG_REQUEST_METHOD

#define YUNLINK_CONFIG_RESPONSE_METHOD(METHOD, TYPE)                                               \
    ErrorCode ConfigurationServicePublisher::METHOD(                                               \
        const EnvelopeEvent& inbound, const TYPE& payload, uint32_t ttl_ms) {                      \
        return runtime_ == nullptr ? ErrorCode::kInvalidArgument                                   \
                                   : runtime_->reply_configuration_service_payload(                \
                                         inbound,                                                  \
                                         MessageTraits<TYPE>::kMessageType,                        \
                                         encode_payload(payload),                                  \
                                         ttl_ms);                                                  \
    }

YUNLINK_CONFIG_RESPONSE_METHOD(publish_resource_list_response, ConfigResourceListResponse)
YUNLINK_CONFIG_RESPONSE_METHOD(publish_resource_describe_response, ConfigResourceDescribeResponse)
YUNLINK_CONFIG_RESPONSE_METHOD(publish_resource_get_response, ConfigResourceGetResponse)
YUNLINK_CONFIG_RESPONSE_METHOD(publish_resource_patch_response, ConfigResourcePatchResponse)
YUNLINK_CONFIG_RESPONSE_METHOD(publish_resource_apply_response, ConfigResourceApplyResponse)

#undef YUNLINK_CONFIG_RESPONSE_METHOD

}  // namespace yunlink
