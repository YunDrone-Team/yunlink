/**
 * @file src/runtime/runtime_authority.cpp
 * @brief Runtime 控制权相关实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

ErrorCode Runtime::request_authority(const std::string& peer_id,
                                     uint64_t session_id,
                                     const TargetSelector& target,
                                     ControlSource source,
                                     uint32_t lease_ttl_ms,
                                     bool allow_preempt) {
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

bool Runtime::current_authority(AuthorityLease* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    if (impl_->authority.state != AuthorityState::kController) {
        return false;
    }
    if (impl_->authority.expires_at_ms > 0 &&
        impl_->authority.expires_at_ms < runtime_now_millis()) {
        return false;
    }
    if (out != nullptr) {
        *out = impl_->authority;
    }
    return true;
}

void Runtime::handle_authority_envelope(const EnvelopeEvent& ev) {
    AuthorityRequest payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        return;
    }

    SessionDescriptor session{};
    if (!describe_session_internal(ev.envelope.session_id, &session) ||
        session.state != SessionState::kActive) {
        return;
    }

    std::lock_guard<std::mutex> lock(impl_->mu);
    if (impl_->authority.expires_at_ms > 0 &&
        impl_->authority.expires_at_ms < runtime_now_millis()) {
        impl_->authority = AuthorityLease{};
    }

    const bool free_control = impl_->authority.state != AuthorityState::kController;
    const bool same_holder = impl_->authority.session_id == ev.envelope.session_id;
    const bool can_preempt = payload.action == AuthorityAction::kPreempt || payload.allow_preempt;

    if (payload.action == AuthorityAction::kRelease && same_holder) {
        impl_->authority = AuthorityLease{};
        return;
    }

    if (payload.action == AuthorityAction::kRenew && same_holder) {
        impl_->authority.lease_ttl_ms = payload.lease_ttl_ms;
        impl_->authority.expires_at_ms = runtime_now_millis() + payload.lease_ttl_ms;
        return;
    }

    if (free_control || same_holder || can_preempt) {
        impl_->authority.state = AuthorityState::kController;
        impl_->authority.session_id = ev.envelope.session_id;
        impl_->authority.target = ev.envelope.target;
        impl_->authority.source = payload.source;
        impl_->authority.lease_ttl_ms = payload.lease_ttl_ms;
        impl_->authority.expires_at_ms = runtime_now_millis() + payload.lease_ttl_ms;
        impl_->authority.peer = ev.peer;
    }
}

}  // namespace sunraycom
