/**
 * @file src/runtime/authority/client.cpp
 * @brief Runtime authority request client operations.
 */

#include "../core/internal.hpp"

namespace yunlink {

ErrorCode Runtime::request_authority(const std::string& peer_id,
                                     uint64_t session_id,
                                     const TargetSelector& target,
                                     ControlSource source,
                                     uint32_t lease_ttl_ms,
                                     bool allow_preempt) {
    if (source == ControlSource::kUnknown) {
        return ErrorCode::kInvalidArgument;
    }
    AuthorityRequest payload{};
    payload.action = allow_preempt ? AuthorityAction::kPreempt : AuthorityAction::kClaim;
    payload.source = source;
    payload.lease_ttl_ms = lease_ttl_ms;
    payload.allow_preempt = allow_preempt;

    SecureEnvelope envelope = make_typed_envelope(config_.self_identity,
                                                  target,
                                                  session_id,
                                                  session_id,
                                                  QosClass::kReliableOrdered,
                                                  payload,
                                                  lease_ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::renew_authority(const std::string& peer_id,
                                   uint64_t session_id,
                                   const TargetSelector& target,
                                   ControlSource source,
                                   uint32_t lease_ttl_ms) {
    if (source == ControlSource::kUnknown) {
        return ErrorCode::kInvalidArgument;
    }
    AuthorityRequest payload{};
    payload.action = AuthorityAction::kRenew;
    payload.source = source;
    payload.lease_ttl_ms = lease_ttl_ms;
    payload.allow_preempt = false;

    SecureEnvelope envelope = make_typed_envelope(config_.self_identity,
                                                  target,
                                                  session_id,
                                                  session_id,
                                                  QosClass::kReliableOrdered,
                                                  payload,
                                                  lease_ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::release_authority(const std::string& peer_id,
                                     uint64_t session_id,
                                     const TargetSelector& target) {
    AuthorityRequest payload{};
    payload.action = AuthorityAction::kRelease;
    payload.source = ControlSource::kGroundStation;
    payload.allow_preempt = false;

    SecureEnvelope envelope = make_typed_envelope(config_.self_identity,
                                                  target,
                                                  session_id,
                                                  session_id,
                                                  QosClass::kReliableOrdered,
                                                  payload,
                                                  1000);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;
    return send_envelope_to_peer(peer_id, envelope);
}

}  // namespace yunlink
