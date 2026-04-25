/**
 * @file src/runtime/runtime_core.cpp
 * @brief Runtime 主分发与生命周期实现。
 */

#include "runtime_internal.hpp"

namespace yunlink {

namespace {

bool envelope_expired(const SecureEnvelope& envelope, uint64_t now_ms) {
    if (envelope.ttl_ms == 0 || envelope.created_at_ms == 0 || now_ms <= envelope.created_at_ms) {
        return false;
    }
    return now_ms - envelope.created_at_ms > envelope.ttl_ms;
}

bool envelope_protocol_version_mismatch(const SecureEnvelope& envelope) {
    return envelope.protocol_major != 1 || envelope.header_version != 1;
}

bool envelope_schema_version_mismatch(const SecureEnvelope& envelope) {
    return envelope.schema_version != 1;
}

bool security_tags_required(const RuntimeConfig& config) {
    return (config.capability_flags & kCapabilitySecurityTags) != 0;
}

void fnv_mix_u8(uint64_t* hash, uint8_t value) {
    *hash ^= value;
    *hash *= 1099511628211ULL;
}

void fnv_mix_u16(uint64_t* hash, uint16_t value) {
    fnv_mix_u8(hash, static_cast<uint8_t>(value & 0xFFU));
    fnv_mix_u8(hash, static_cast<uint8_t>((value >> 8) & 0xFFU));
}

void fnv_mix_u32(uint64_t* hash, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        fnv_mix_u8(hash, static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

void fnv_mix_u64(uint64_t* hash, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        fnv_mix_u8(hash, static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

void fnv_mix_bytes(uint64_t* hash, const ByteBuffer& bytes) {
    for (uint8_t byte : bytes) {
        fnv_mix_u8(hash, byte);
    }
}

void fnv_mix_string(uint64_t* hash, const std::string& value) {
    for (char ch : value) {
        fnv_mix_u8(hash, static_cast<uint8_t>(ch));
    }
}

ByteBuffer make_runtime_auth_tag(const RuntimeConfig& config, const SecureEnvelope& envelope) {
    uint64_t hash = 1469598103934665603ULL;
    fnv_mix_string(&hash, config.shared_secret);
    fnv_mix_u32(&hash, envelope.security.key_epoch);
    fnv_mix_u8(&hash, envelope.protocol_major);
    fnv_mix_u8(&hash, envelope.header_version);
    fnv_mix_u16(&hash, envelope.flags);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.qos_class));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.message_family));
    fnv_mix_u16(&hash, envelope.message_type);
    fnv_mix_u16(&hash, envelope.schema_version);
    fnv_mix_u64(&hash, envelope.session_id);
    fnv_mix_u64(&hash, envelope.message_id);
    fnv_mix_u64(&hash, envelope.correlation_id);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.source.agent_type));
    fnv_mix_u32(&hash, envelope.source.agent_id);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.source.role));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.target.scope));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.target.target_type));
    fnv_mix_u32(&hash, envelope.target.group_id);
    for (uint32_t id : envelope.target.target_ids) {
        fnv_mix_u32(&hash, id);
    }
    fnv_mix_u64(&hash, envelope.created_at_ms);
    fnv_mix_u32(&hash, envelope.ttl_ms);
    fnv_mix_bytes(&hash, envelope.payload);

    ByteBuffer tag(8);
    for (int i = 0; i < 8; ++i) {
        tag[static_cast<size_t>(i)] = static_cast<uint8_t>((hash >> (i * 8)) & 0xFFU);
    }
    return tag;
}

void apply_runtime_security_tag(const RuntimeConfig& config, SecureEnvelope* envelope) {
    if (!security_tags_required(config) || envelope == nullptr) {
        return;
    }
    envelope->security.key_epoch = config.security_key_epoch;
    envelope->security.auth_tag = make_runtime_auth_tag(config, *envelope);
}

std::string runtime_security_replay_key(const SecureEnvelope& envelope) {
    return std::to_string(envelope.security.key_epoch) + ":" +
           std::to_string(static_cast<uint8_t>(envelope.source.agent_type)) + ":" +
           std::to_string(envelope.source.agent_id) + ":" + std::to_string(envelope.message_id);
}

}  // namespace

Runtime::Runtime()
    : impl_(std::make_unique<Impl>()), session_client_(this), session_server_(this),
      command_publisher_(this), command_subscriber_(this), state_subscriber_(this),
      event_subscriber_(this) {}

Runtime::~Runtime() {
    stop();
}

ErrorCode Runtime::start(const RuntimeConfig& config) {
    if (is_started_) {
        return ErrorCode::kOk;
    }
    config_ = config;

    const ErrorCode ec_udp = udp_.start(config_, &bus_);
    if (ec_udp != ErrorCode::kOk) {
        return ec_udp;
    }

    const ErrorCode ec_clients = tcp_clients_.start(config_, &bus_);
    if (ec_clients != ErrorCode::kOk) {
        udp_.stop();
        return ec_clients;
    }

    const ErrorCode ec_server = tcp_server_.start(config_, &bus_);
    if (ec_server != ErrorCode::kOk) {
        tcp_clients_.stop();
        udp_.stop();
        return ec_server;
    }

    impl_->bus_token =
        bus_.subscribe_envelope([this](const EnvelopeEvent& ev) { handle_envelope(ev); });
    impl_->link_bus_token = bus_.subscribe_link([this](const LinkEvent& ev) {
        handle_link_event(ev);
    });
    is_started_ = true;
    return ErrorCode::kOk;
}

void Runtime::stop() {
    if (!is_started_) {
        return;
    }
    bus_.unsubscribe(impl_->bus_token);
    bus_.unsubscribe(impl_->link_bus_token);
    tcp_server_.stop();
    tcp_clients_.stop();
    udp_.stop();
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        impl_->sessions.clear();
        impl_->authorities.clear();
        impl_->active_bulk_channels.clear();
        impl_->reliable_latest_watermarks.clear();
        impl_->trajectory_accumulators.clear();
        impl_->security_replay_keys.clear();
    }
    impl_->bus_token = 0;
    impl_->link_bus_token = 0;
    is_started_ = false;
}

uint64_t Runtime::allocate_session_id() {
    std::lock_guard<std::mutex> lock(impl_->mu);
    return impl_->next_session_id++;
}

uint64_t Runtime::allocate_message_id() {
    std::lock_guard<std::mutex> lock(impl_->mu);
    return impl_->next_message_id++;
}

ErrorCode Runtime::send_envelope_to_peer(const std::string& peer_id,
                                         const SecureEnvelope& envelope) {
    if (!is_started_) {
        return ErrorCode::kRuntimeStopped;
    }
    SecureEnvelope outbound = envelope;
    apply_runtime_security_tag(config_, &outbound);
    const int sent_client = tcp_clients_.send_envelope(peer_id, outbound);
    if (sent_client >= 0) {
        return ErrorCode::kOk;
    }
    const int sent_server = tcp_server_.send_envelope(peer_id, outbound);
    return sent_server >= 0 ? ErrorCode::kOk : ErrorCode::kConnectError;
}

ErrorCode Runtime::reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope) {
    if (!is_started_) {
        return ErrorCode::kRuntimeStopped;
    }
    SecureEnvelope outbound = envelope;
    apply_runtime_security_tag(config_, &outbound);
    if (inbound.transport == TransportType::kTcpServer) {
        return tcp_server_.send_envelope(inbound.peer.id, outbound) >= 0
                   ? ErrorCode::kOk
                   : ErrorCode::kConnectError;
    }
    if (inbound.transport == TransportType::kTcpClient) {
        return tcp_clients_.send_envelope(inbound.peer.id, outbound) >= 0
                   ? ErrorCode::kOk
                   : ErrorCode::kConnectError;
    }
    return udp_.send_envelope_unicast(outbound, inbound.peer.ip, inbound.peer.port) >= 0
               ? ErrorCode::kOk
               : ErrorCode::kConnectError;
}

bool Runtime::describe_session_internal(uint64_t session_id, SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    for (const auto& entry : impl_->sessions) {
        if (entry.second.session_id != session_id) {
            continue;
        }
        if (out != nullptr) {
            *out = entry.second;
        }
        return true;
    }
    return false;
}

bool Runtime::describe_session_internal(const std::string& peer_id,
                                        uint64_t session_id,
                                        SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->sessions.find(runtime_session_key(peer_id, session_id));
    if (it == impl_->sessions.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second;
    }
    return true;
}

bool Runtime::find_active_session_internal(SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    for (const auto& entry : impl_->sessions) {
        if (entry.second.state != SessionState::kActive) {
            continue;
        }
        if (out != nullptr) {
            *out = entry.second;
        }
        return true;
    }
    return false;
}

void Runtime::handle_envelope(const EnvelopeEvent& ev) {
    const auto reject_command_before_dispatch = [&](ErrorCode code, const std::string& detail) {
        if (ev.envelope.message_family != MessageFamily::kCommand ||
            !ev.envelope.target.matches(config_.self_identity)) {
            return;
        }

        CommandResult result{};
        result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
        result.phase = CommandPhase::kFailed;
        result.result_code = static_cast<uint16_t>(code);
        result.progress_percent = 0;
        result.detail = detail;

        SecureEnvelope reply =
            make_typed_envelope(config_.self_identity,
                                TargetSelector::for_entity(ev.envelope.source.agent_type,
                                                           ev.envelope.source.agent_id),
                                ev.envelope.session_id,
                                ev.envelope.message_id,
                                QosClass::kReliableOrdered,
                                result,
                                1000);
        reply.message_id = allocate_message_id();
        reply.correlation_id = ev.envelope.message_id;
        (void)reply_on_route(ev, reply);
    };

    if (envelope_protocol_version_mismatch(ev.envelope)) {
        ErrorEvent error;
        error.code = ErrorCode::kProtocolMismatch;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-protocol-version-mismatch";
        bus_.publish_error(error);
        reject_command_before_dispatch(ErrorCode::kProtocolMismatch,
                                       "runtime-protocol-version-mismatch");
        return;
    }

    if (envelope_schema_version_mismatch(ev.envelope)) {
        ErrorEvent error;
        error.code = ErrorCode::kProtocolMismatch;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-schema-version-mismatch";
        bus_.publish_error(error);
        reject_command_before_dispatch(ErrorCode::kProtocolMismatch,
                                       "runtime-schema-version-mismatch");
        return;
    }

    if (security_tags_required(config_)) {
        const auto publish_security_error = [&](const std::string& detail) {
            ErrorEvent error;
            error.code = ErrorCode::kUnauthorized;
            error.transport = ev.transport;
            error.peer = ev.peer;
            error.message = detail;
            bus_.publish_error(error);
        };

        if (ev.envelope.security.key_epoch != config_.security_key_epoch) {
            publish_security_error("security-key-epoch-mismatch");
            return;
        }
        if (ev.envelope.security.auth_tag != make_runtime_auth_tag(config_, ev.envelope)) {
            publish_security_error("security-auth-tag-mismatch");
            return;
        }

        const std::string replay_key = runtime_security_replay_key(ev.envelope);
        {
            std::lock_guard<std::mutex> lock(impl_->mu);
            if (impl_->security_replay_keys.find(replay_key) != impl_->security_replay_keys.end()) {
                publish_security_error("security-replay-detected");
                return;
            }
            impl_->security_replay_keys.insert(replay_key);
        }
    }

    if (envelope_expired(ev.envelope, runtime_now_millis())) {
        ErrorEvent error;
        error.code = ErrorCode::kTimeout;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-ttl-expired";
        bus_.publish_error(error);

        if (ev.envelope.message_family == MessageFamily::kCommand &&
            ev.envelope.target.matches(config_.self_identity)) {
            CommandResult result{};
            result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
            result.phase = CommandPhase::kExpired;
            result.result_code = static_cast<uint16_t>(ErrorCode::kTimeout);
            result.progress_percent = 0;
            result.detail = "runtime-ttl-expired";

            SecureEnvelope reply =
                make_typed_envelope(config_.self_identity,
                                    TargetSelector::for_entity(ev.envelope.source.agent_type,
                                                               ev.envelope.source.agent_id),
                                    ev.envelope.session_id,
                                    ev.envelope.message_id,
                                    QosClass::kReliableOrdered,
                                    result,
                                    1000);
            reply.message_id = allocate_message_id();
            reply.correlation_id = ev.envelope.message_id;
            (void)reply_on_route(ev, reply);
        }
        return;
    }

    switch (ev.envelope.message_family) {
    case MessageFamily::kSession:
        handle_session_envelope(ev);
        return;
    case MessageFamily::kAuthority:
        handle_authority_envelope(ev);
        return;
    case MessageFamily::kCommand:
        handle_command_envelope(ev);
        return;
    case MessageFamily::kStateSnapshot:
        handle_state_snapshot_envelope(ev);
        return;
    case MessageFamily::kStateEvent:
        handle_state_event_envelope(ev);
        return;
    case MessageFamily::kCommandResult:
        handle_command_result_envelope(ev);
        return;
    case MessageFamily::kBulkChannelDescriptor:
        handle_bulk_channel_descriptor_envelope(ev);
        return;
    }
}

void Runtime::handle_link_event(const LinkEvent& ev) {
    if (ev.is_up) {
        return;
    }

    std::lock_guard<std::mutex> lock(impl_->mu);
    for (auto& entry : impl_->sessions) {
        SessionDescriptor& session = entry.second;
        if (session.peer.id != ev.peer.id) {
            continue;
        }
        if (session.state == SessionState::kClosed || session.state == SessionState::kInvalid) {
            continue;
        }
        session.state = SessionState::kLost;
    }

    for (auto it = impl_->authorities.begin(); it != impl_->authorities.end();) {
        if (it->second.peer.id == ev.peer.id) {
            it = impl_->authorities.erase(it);
        } else {
            ++it;
        }
    }

    const std::string session_key_prefix = ev.peer.id + "#";
    for (auto it = impl_->trajectory_accumulators.begin();
         it != impl_->trajectory_accumulators.end();) {
        if (it->first.rfind(session_key_prefix, 0) == 0) {
            it = impl_->trajectory_accumulators.erase(it);
        } else {
            ++it;
        }
    }

    impl_->active_bulk_channels.clear();
    impl_->security_replay_keys.clear();
}

}  // namespace yunlink
