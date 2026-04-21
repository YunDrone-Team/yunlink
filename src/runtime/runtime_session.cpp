/**
 * @file src/runtime/runtime_session.cpp
 * @brief Runtime 会话相关实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

SessionClient::SessionClient(Runtime* runtime) : runtime_(runtime) {}

void SessionClient::bind(Runtime* runtime) {
    runtime_ = runtime;
}

uint64_t SessionClient::open_active_session(const std::string& peer_id,
                                            const std::string& node_name) {
    if (runtime_ == nullptr) {
        return 0;
    }

    const uint64_t session_id = runtime_->allocate_session_id();
    const uint64_t root_correlation_id = runtime_->allocate_message_id();

    SessionHello hello{};
    hello.node_name = node_name;
    hello.capability_flags = runtime_->config_.capability_flags;
    if (runtime_->send_session_payload(peer_id,
                                       session_id,
                                       root_correlation_id,
                                       static_cast<uint16_t>(SessionMessageType::kHello),
                                       encode_payload(hello),
                                       1000) != ErrorCode::kOk) {
        return 0;
    }

    SessionAuthenticate auth{};
    auth.shared_secret = runtime_->config_.shared_secret;
    runtime_->send_session_payload(peer_id,
                                   session_id,
                                   root_correlation_id,
                                   static_cast<uint16_t>(SessionMessageType::kAuthenticate),
                                   encode_payload(auth),
                                   1000);

    SessionCapabilities caps{};
    caps.capability_flags = runtime_->config_.capability_flags;
    runtime_->send_session_payload(peer_id,
                                   session_id,
                                   root_correlation_id,
                                   static_cast<uint16_t>(SessionMessageType::kCapabilities),
                                   encode_payload(caps),
                                   1000);

    SessionReady ready{};
    ready.accepted_protocol_major = 1;
    runtime_->send_session_payload(peer_id,
                                   session_id,
                                   root_correlation_id,
                                   static_cast<uint16_t>(SessionMessageType::kReady),
                                   encode_payload(ready),
                                   1000);
    return session_id;
}

SessionServer::SessionServer(Runtime* runtime) : runtime_(runtime) {}

void SessionServer::bind(Runtime* runtime) {
    runtime_ = runtime;
}

bool SessionServer::has_active_session(uint64_t session_id) const {
    SessionDescriptor desc;
    return describe_session(session_id, &desc) && desc.state == SessionState::kActive;
}

bool SessionServer::describe_session(uint64_t session_id, SessionDescriptor* out) const {
    return runtime_ != nullptr && runtime_->describe_session_internal(session_id, out);
}

ErrorCode Runtime::send_session_payload(const std::string& peer_id,
                                        uint64_t session_id,
                                        uint64_t correlation_id,
                                        uint16_t message_type,
                                        const ByteBuffer& payload,
                                        uint32_t ttl_ms) {
    SecureEnvelope envelope = make_runtime_envelope(config_.self_identity,
                                                    TargetSelector::broadcast(AgentType::kUnknown),
                                                    session_id,
                                                    correlation_id,
                                                    QosClass::kReliableOrdered,
                                                    MessageFamily::kSession,
                                                    message_type,
                                                    payload,
                                                    ttl_ms);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

void Runtime::handle_session_envelope(const EnvelopeEvent& ev) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    SessionDescriptor& session = impl_->sessions[ev.envelope.session_id];
    session.session_id = ev.envelope.session_id;
    session.peer = ev.peer;
    session.remote_identity = ev.envelope.source;

    if (ev.envelope.message_type == static_cast<uint16_t>(SessionMessageType::kHello)) {
        SessionHello payload{};
        if (decode_typed_payload(ev.envelope.payload, &payload)) {
            session.state = SessionState::kHandshaking;
            session.node_name = payload.node_name;
            session.capability_flags = payload.capability_flags;
        }
        return;
    }

    if (ev.envelope.message_type == static_cast<uint16_t>(SessionMessageType::kAuthenticate)) {
        SessionAuthenticate payload{};
        if (decode_typed_payload(ev.envelope.payload, &payload)) {
            session.state = payload.shared_secret == config_.shared_secret
                                ? SessionState::kAuthenticated
                                : SessionState::kClosed;
        }
        return;
    }

    if (ev.envelope.message_type == static_cast<uint16_t>(SessionMessageType::kCapabilities)) {
        SessionCapabilities payload{};
        if (decode_typed_payload(ev.envelope.payload, &payload) &&
            session.state == SessionState::kAuthenticated) {
            session.state = SessionState::kNegotiated;
            session.capability_flags = payload.capability_flags;
        }
        return;
    }

    if (ev.envelope.message_type == static_cast<uint16_t>(SessionMessageType::kReady)) {
        SessionReady payload{};
        if (decode_typed_payload(ev.envelope.payload, &payload) &&
            session.state == SessionState::kNegotiated &&
            payload.accepted_protocol_major == ev.envelope.protocol_major) {
            session.state = SessionState::kActive;
        }
    }
}

}  // namespace sunraycom
