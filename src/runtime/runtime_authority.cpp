/**
 * @file src/runtime/runtime_authority.cpp
 * @brief Runtime 控制权相关实现。
 */

#include "runtime_internal.hpp"

namespace yunlink {

namespace {

constexpr uint16_t kAuthorityReasonOk = 0;
constexpr uint16_t kAuthorityReasonRejected = static_cast<uint16_t>(ErrorCode::kRejected);
constexpr uint16_t kAuthorityReasonTimeout = static_cast<uint16_t>(ErrorCode::kTimeout);

struct PendingAuthorityStatus {
    std::string peer_id;
    TargetSelector target;
    uint64_t session_id = 0;
    AuthorityStatus status;
};

AuthorityStatus make_authority_status(AuthorityState state,
                                      uint64_t session_id,
                                      uint32_t lease_ttl_ms,
                                      uint16_t reason_code,
                                      const std::string& detail) {
    AuthorityStatus status{};
    status.state = state;
    status.session_id = session_id;
    status.lease_ttl_ms = lease_ttl_ms;
    status.reason_code = reason_code;
    status.detail = detail;
    return status;
}

}  // namespace

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

bool Runtime::current_authority(AuthorityLease* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const uint64_t now_ms = runtime_now_millis();
    for (const auto& item : impl_->authorities) {
        const AuthorityLease& lease = item.second;
        if (lease.state != AuthorityState::kController) {
            continue;
        }
        if (lease.expires_at_ms > 0 && lease.expires_at_ms < now_ms) {
            continue;
        }
        if (out != nullptr) {
            *out = lease;
        }
        return true;
    }
    return false;
}

bool Runtime::current_authority_for_target(const TargetSelector& target,
                                           AuthorityLease* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->authorities.find(runtime_target_key(target));
    if (it == impl_->authorities.end()) {
        return false;
    }
    const AuthorityLease& lease = it->second;
    if (lease.state != AuthorityState::kController) {
        return false;
    }
    if (lease.expires_at_ms > 0 && lease.expires_at_ms < runtime_now_millis()) {
        return false;
    }
    if (out != nullptr) {
        *out = lease;
    }
    return true;
}

void Runtime::handle_authority_envelope(const EnvelopeEvent& ev) {
    if (ev.envelope.message_type == static_cast<uint16_t>(AuthorityMessageType::kStatus)) {
        if (!ev.envelope.target.matches(config_.self_identity)) {
            return;
        }
        AuthorityStatus payload{};
        if (!decode_typed_payload(ev.envelope.payload, &payload)) {
            return;
        }

        TypedMessage<AuthorityStatus> message{ev.envelope, payload};
        std::unordered_map<size_t, EventSubscriber::AuthorityStatusHandler> handlers;
        {
            std::lock_guard<std::mutex> lock(impl_->mu);
            handlers = impl_->authority_status_handlers;
        }
        for (const auto& item : handlers) {
            if (item.second) {
                item.second(message);
            }
        }
        return;
    }

    AuthorityRequest payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        return;
    }

    const auto reply_to_requester = [&](const AuthorityStatus& status) {
        SecureEnvelope reply = make_typed_envelope(
            config_.self_identity,
            TargetSelector::for_entity(ev.envelope.source.agent_type, ev.envelope.source.agent_id),
            ev.envelope.session_id,
            ev.envelope.message_id,
            QosClass::kReliableOrdered,
            status,
            1000);
        reply.message_id = allocate_message_id();
        reply.correlation_id = ev.envelope.message_id;
        (void)reply_on_route(ev, reply);
    };

    const auto send_to_peer = [&](const PendingAuthorityStatus& pending) {
        SecureEnvelope envelope = make_typed_envelope(config_.self_identity,
                                                      pending.target,
                                                      pending.session_id,
                                                      ev.envelope.message_id,
                                                      QosClass::kReliableOrdered,
                                                      pending.status,
                                                      1000);
        envelope.message_id = allocate_message_id();
        envelope.correlation_id = ev.envelope.message_id;
        (void)send_envelope_to_peer(pending.peer_id, envelope);
    };

    if (payload.source == ControlSource::kUnknown) {
        reply_to_requester(make_authority_status(AuthorityState::kRejected,
                                                 ev.envelope.session_id,
                                                 payload.lease_ttl_ms,
                                                 kAuthorityReasonRejected,
                                                 "invalid-authority-source"));
        return;
    }

    SessionDescriptor session{};
    if (!describe_session_internal(ev.peer.id, ev.envelope.session_id, &session) ||
        session.state != SessionState::kActive) {
        reply_to_requester(make_authority_status(AuthorityState::kRejected,
                                                 ev.envelope.session_id,
                                                 payload.lease_ttl_ms,
                                                 kAuthorityReasonRejected,
                                                 "no-active-session"));
        return;
    }

    std::vector<PendingAuthorityStatus> pending_statuses;
    AuthorityStatus requester_status{};
    bool has_requester_status = false;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        const std::string target_key = runtime_target_key(ev.envelope.target);
        auto it = impl_->authorities.find(target_key);
        if (it != impl_->authorities.end() && it->second.expires_at_ms > 0 &&
            it->second.expires_at_ms < runtime_now_millis()) {
            pending_statuses.push_back(PendingAuthorityStatus{
                it->second.peer.id,
                TargetSelector::broadcast(AgentType::kGroundStation),
                it->second.session_id,
                make_authority_status(AuthorityState::kRevoked,
                                      it->second.session_id,
                                      it->second.lease_ttl_ms,
                                      kAuthorityReasonTimeout,
                                      "authority-expired"),
            });
            it = impl_->authorities.erase(it);
        }

        AuthorityLease& lease = impl_->authorities[target_key];
        const bool free_control = lease.state != AuthorityState::kController;
        const bool same_holder =
            lease.session_id == ev.envelope.session_id && lease.peer.id == ev.peer.id;
        const bool can_preempt =
            payload.action == AuthorityAction::kPreempt || payload.allow_preempt;

        if (payload.action == AuthorityAction::kRelease && same_holder) {
            impl_->authorities.erase(target_key);
            requester_status = make_authority_status(AuthorityState::kReleased,
                                                     ev.envelope.session_id,
                                                     0,
                                                     kAuthorityReasonOk,
                                                     "authority-released");
            has_requester_status = true;
        } else if (payload.action == AuthorityAction::kRelease) {
            requester_status = make_authority_status(AuthorityState::kRejected,
                                                     ev.envelope.session_id,
                                                     0,
                                                     kAuthorityReasonRejected,
                                                     "authority-not-holder");
            has_requester_status = true;
        } else if (payload.action == AuthorityAction::kRenew && same_holder) {
            lease.lease_ttl_ms = payload.lease_ttl_ms;
            lease.expires_at_ms = runtime_now_millis() + payload.lease_ttl_ms;
            requester_status = make_authority_status(AuthorityState::kController,
                                                     ev.envelope.session_id,
                                                     payload.lease_ttl_ms,
                                                     kAuthorityReasonOk,
                                                     "authority-renewed");
            has_requester_status = true;
        } else if (payload.action == AuthorityAction::kRenew) {
            requester_status = make_authority_status(AuthorityState::kRejected,
                                                     ev.envelope.session_id,
                                                     payload.lease_ttl_ms,
                                                     kAuthorityReasonRejected,
                                                     "authority-not-holder");
            has_requester_status = true;
        } else if (free_control || same_holder || can_preempt) {
            if (!free_control && !same_holder && can_preempt) {
                pending_statuses.push_back(PendingAuthorityStatus{
                    lease.peer.id,
                    TargetSelector::broadcast(AgentType::kGroundStation),
                    lease.session_id,
                    make_authority_status(AuthorityState::kRevoked,
                                          lease.session_id,
                                          lease.lease_ttl_ms,
                                          kAuthorityReasonRejected,
                                          "authority-preempted"),
                });
            }
            lease.state = AuthorityState::kController;
            lease.session_id = ev.envelope.session_id;
            lease.target = ev.envelope.target;
            lease.source = payload.source;
            lease.lease_ttl_ms = payload.lease_ttl_ms;
            lease.expires_at_ms = runtime_now_millis() + payload.lease_ttl_ms;
            lease.peer = ev.peer;
            requester_status = make_authority_status(AuthorityState::kController,
                                                     ev.envelope.session_id,
                                                     payload.lease_ttl_ms,
                                                     kAuthorityReasonOk,
                                                     "authority-granted");
            has_requester_status = true;
        } else {
            requester_status = make_authority_status(AuthorityState::kRejected,
                                                     ev.envelope.session_id,
                                                     payload.lease_ttl_ms,
                                                     kAuthorityReasonRejected,
                                                     "authority-held");
            has_requester_status = true;
        }
    }

    for (const auto& pending : pending_statuses) {
        send_to_peer(pending);
    }
    if (has_requester_status) {
        reply_to_requester(requester_status);
    }
}

}  // namespace yunlink
