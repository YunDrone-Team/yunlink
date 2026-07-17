/**
 * @file src/runtime/system_service/publish.cpp
 * @brief Runtime system service publishing implementation.
 */

#include "../core/internal.hpp"

namespace yunlink {

namespace {

void fill_system_service_handle(SystemServiceHandle* out_handle,
                                uint64_t session_id,
                                uint64_t message_id,
                                uint64_t correlation_id,
                                const TargetSelector& target) {
    if (out_handle == nullptr) {
        return;
    }
    out_handle->session_id = session_id;
    out_handle->message_id = message_id;
    out_handle->correlation_id = correlation_id;
    out_handle->target = target;
}

}  // namespace

SystemServicePublisher::SystemServicePublisher(Runtime* runtime) : runtime_(runtime) {}

void SystemServicePublisher::bind(Runtime* runtime) {
    runtime_ = runtime;
}

ErrorCode Runtime::publish_system_service_request_payload(const std::string& peer_id,
                                                          uint64_t session_id,
                                                          const TargetSelector& target,
                                                          uint16_t message_type,
                                                          const ByteBuffer& payload,
                                                          SystemServiceHandle* out_handle,
                                                          uint32_t ttl_ms) {
    SessionDescriptor session{};
    if (describe_session_internal(peer_id, session_id, &session) &&
        (session.state == SessionState::kLost || session.state == SessionState::kInvalid ||
         session.state == SessionState::kClosed)) {
        return ErrorCode::kRejected;
    }

    SecureEnvelope envelope = make_runtime_envelope(config_.self_identity,
                                                    target,
                                                    session_id,
                                                    0,
                                                    QosClass::kReliableOrdered,
                                                    MessageFamily::kSystemService,
                                                    message_type,
                                                    payload,
                                                    ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;

    const ErrorCode ec = send_envelope_to_peer(peer_id, envelope);
    if (ec == ErrorCode::kOk) {
        fill_system_service_handle(
            out_handle, session_id, envelope.message_id, envelope.correlation_id, target);
    }
    return ec;
}

ErrorCode Runtime::reply_system_service_payload(const EnvelopeEvent& inbound,
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
                              MessageFamily::kSystemService,
                              message_type,
                              payload,
                              ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = inbound.envelope.message_id;
    return reply_on_route(inbound, envelope);
}

ErrorCode SystemServicePublisher::publish_feature_list_request(const std::string& peer_id,
                                                               uint64_t session_id,
                                                               const TargetSelector& target,
                                                               const FeatureListRequest& payload,
                                                               SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<FeatureListRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     3000);
}

ErrorCode SystemServicePublisher::publish_feature_get_request(const std::string& peer_id,
                                                              uint64_t session_id,
                                                              const TargetSelector& target,
                                                              const FeatureGetRequest& payload,
                                                              SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<FeatureGetRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     3000);
}

ErrorCode SystemServicePublisher::publish_feature_start_request(const std::string& peer_id,
                                                                uint64_t session_id,
                                                                const TargetSelector& target,
                                                                const FeatureStartRequest& payload,
                                                                SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<FeatureStartRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     3000);
}

ErrorCode SystemServicePublisher::publish_feature_stop_request(const std::string& peer_id,
                                                               uint64_t session_id,
                                                               const TargetSelector& target,
                                                               const FeatureStopRequest& payload,
                                                               SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<FeatureStopRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     3000);
}

ErrorCode SystemServicePublisher::publish_feature_list_response(const EnvelopeEvent& inbound,
                                                                const FeatureListResponse& payload,
                                                                uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<FeatureListResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

ErrorCode SystemServicePublisher::publish_feature_get_response(const EnvelopeEvent& inbound,
                                                               const FeatureGetResponse& payload,
                                                               uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<FeatureGetResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

ErrorCode
SystemServicePublisher::publish_feature_start_response(const EnvelopeEvent& inbound,
                                                       const FeatureStartResponse& payload,
                                                       uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<FeatureStartResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

ErrorCode SystemServicePublisher::publish_feature_stop_response(const EnvelopeEvent& inbound,
                                                                const FeatureStopResponse& payload,
                                                                uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<FeatureStopResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

ErrorCode
SystemServicePublisher::publish_runtime_log_list_request(const std::string& peer_id,
                                                         uint64_t session_id,
                                                         const TargetSelector& target,
                                                         const RuntimeLogListRequest& payload,
                                                         SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<RuntimeLogListRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     5000);
}

ErrorCode
SystemServicePublisher::publish_runtime_log_read_request(const std::string& peer_id,
                                                         uint64_t session_id,
                                                         const TargetSelector& target,
                                                         const RuntimeLogReadRequest& payload,
                                                         SystemServiceHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_system_service_request_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<RuntimeLogReadRequest>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     5000);
}

ErrorCode
SystemServicePublisher::publish_runtime_log_list_response(const EnvelopeEvent& inbound,
                                                          const RuntimeLogListResponse& payload,
                                                          uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<RuntimeLogListResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

ErrorCode
SystemServicePublisher::publish_runtime_log_read_response(const EnvelopeEvent& inbound,
                                                          const RuntimeLogReadResponse& payload,
                                                          uint32_t ttl_ms) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->reply_system_service_payload(
                                     inbound,
                                     MessageTraits<RuntimeLogReadResponse>::kMessageType,
                                     encode_payload(payload),
                                     ttl_ms);
}

}  // namespace yunlink
