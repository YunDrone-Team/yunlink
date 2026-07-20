/**
 * @file src/runtime/core/dispatch.cpp
 * @brief Runtime envelope dispatch and link cleanup.
 */

#include "security.hpp"

namespace yunlink {
namespace {

CommandResult make_rejected_result(const EnvelopeEvent& ev,
                                   ErrorCode code,
                                   CommandPhase phase,
                                   const std::string& detail) {
    CommandResult result{};
    result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
    result.phase = phase;
    result.result_code = static_cast<uint16_t>(code);
    result.progress_percent = 0;
    result.detail = detail;
    return result;
}

}  // namespace

void Runtime::handle_envelope(const EnvelopeEvent& ev) {
    const auto trace_dispatch =
        [&](PacketTraceStage stage, ErrorCode code, const std::string& detail) {
            runtime_publish_packet_trace(bus_,
                                         config_,
                                         PacketTraceDirection::kRx,
                                         stage,
                                         ev.transport,
                                         ev.peer,
                                         ev.envelope,
                                         nullptr,
                                         0,
                                         code,
                                         detail);
        };

    const auto reject_command_before_dispatch = [&](ErrorCode code, const std::string& detail) {
        if (ev.envelope.message_family != MessageFamily::kCommand ||
            !matches_local_target(ev.envelope.target)) {
            return;
        }

        SecureEnvelope reply = make_typed_envelope(
            source_for_target(ev.envelope.target),
            TargetSelector::for_entity(ev.envelope.source.agent_type, ev.envelope.source.agent_id),
            ev.envelope.session_id,
            ev.envelope.message_id,
            QosClass::kReliableOrdered,
            make_rejected_result(ev, code, CommandPhase::kFailed, detail),
            1000);
        reply.message_id = allocate_message_id();
        reply.correlation_id = ev.envelope.message_id;
        (void)reply_on_route(ev, reply);
    };

    if (runtime_protocol_version_mismatch(ev.envelope)) {
        ErrorEvent error;
        error.code = ErrorCode::kProtocolMismatch;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-protocol-version-mismatch";
        bus_.publish_error(error);
        reject_command_before_dispatch(ErrorCode::kProtocolMismatch,
                                       "runtime-protocol-version-mismatch");
        trace_dispatch(PacketTraceStage::kDispatchRejected,
                       ErrorCode::kProtocolMismatch,
                       "runtime-protocol-version-mismatch");
        return;
    }

    if (runtime_schema_version_mismatch(ev.envelope)) {
        ErrorEvent error;
        error.code = ErrorCode::kProtocolMismatch;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-schema-version-mismatch";
        bus_.publish_error(error);
        reject_command_before_dispatch(ErrorCode::kProtocolMismatch,
                                       "runtime-schema-version-mismatch");
        trace_dispatch(PacketTraceStage::kDispatchRejected,
                       ErrorCode::kProtocolMismatch,
                       "runtime-schema-version-mismatch");
        return;
    }

    const bool has_security_tag = !ev.envelope.security.auth_tag.empty();
    if (runtime_security_tags_required(config_) ||
        (runtime_security_tags_enabled(config_) && has_security_tag)) {
        const auto publish_security_error = [&](const std::string& detail) {
            ErrorEvent error;
            error.code = ErrorCode::kUnauthorized;
            error.transport = ev.transport;
            error.peer = ev.peer;
            error.message = detail;
            bus_.publish_error(error);
        };

        if (!has_security_tag) {
            publish_security_error("security-auth-tag-missing");
            trace_dispatch(PacketTraceStage::kDispatchRejected,
                           ErrorCode::kUnauthorized,
                           "security-auth-tag-missing");
            return;
        }
        if (ev.envelope.security.key_epoch != config_.security_key_epoch) {
            publish_security_error("security-key-epoch-mismatch");
            trace_dispatch(PacketTraceStage::kDispatchRejected,
                           ErrorCode::kUnauthorized,
                           "security-key-epoch-mismatch");
            return;
        }
        if (ev.envelope.security.auth_tag != make_runtime_auth_tag(config_, ev.envelope)) {
            publish_security_error("security-auth-tag-mismatch");
            trace_dispatch(PacketTraceStage::kDispatchRejected,
                           ErrorCode::kUnauthorized,
                           "security-auth-tag-mismatch");
            return;
        }

        const std::string replay_key = runtime_security_replay_key(ev.envelope);
        {
            std::lock_guard<std::mutex> lock(impl_->mu);
            if (impl_->security_replay_keys.find(replay_key) != impl_->security_replay_keys.end()) {
                publish_security_error("security-replay-detected");
                trace_dispatch(PacketTraceStage::kDispatchRejected,
                               ErrorCode::kUnauthorized,
                               "security-replay-detected");
                return;
            }
            impl_->security_replay_keys.insert(replay_key);
        }
    }

    if (runtime_envelope_expired(ev.envelope, runtime_now_millis())) {
        ErrorEvent error;
        error.code = ErrorCode::kTimeout;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "runtime-ttl-expired";
        bus_.publish_error(error);

        if (ev.envelope.message_family == MessageFamily::kCommand &&
            matches_local_target(ev.envelope.target)) {
            SecureEnvelope reply = make_typed_envelope(
                source_for_target(ev.envelope.target),
                TargetSelector::for_entity(ev.envelope.source.agent_type,
                                           ev.envelope.source.agent_id),
                ev.envelope.session_id,
                ev.envelope.message_id,
                QosClass::kReliableOrdered,
                make_rejected_result(
                    ev, ErrorCode::kTimeout, CommandPhase::kExpired, "runtime-ttl-expired"),
                1000);
            reply.message_id = allocate_message_id();
            reply.correlation_id = ev.envelope.message_id;
            (void)reply_on_route(ev, reply);
        }
        trace_dispatch(
            PacketTraceStage::kDispatchRejected, ErrorCode::kTimeout, "runtime-ttl-expired");
        return;
    }

    if (ev.envelope.message_family != MessageFamily::kSession) {
        SessionDescriptor session{};
        const bool has_session =
            describe_session_internal(ev.peer.id, ev.envelope.session_id, &session) ||
            describe_session_internal(ev.envelope.session_id, &session);
        if (has_session &&
            (session.state != SessionState::kActive || !session.authenticated ||
             !runtime_remote_source_allowed(session, ev.envelope.source))) {
            ErrorEvent error;
            error.code = ErrorCode::kUnauthorized;
            error.transport = ev.transport;
            error.peer = ev.peer;
            error.message = "session-source-not-authorized";
            bus_.publish_error(error);
            trace_dispatch(PacketTraceStage::kDispatchRejected,
                           ErrorCode::kUnauthorized,
                           "session-source-not-authorized");
            return;
        }
    }

    if (!matches_local_target(ev.envelope.target) &&
        ev.envelope.message_family != MessageFamily::kSession) {
        trace_dispatch(
            PacketTraceStage::kDispatchRejected, ErrorCode::kRejected, "target-mismatch");
    } else {
        trace_dispatch(PacketTraceStage::kDispatchAccepted, ErrorCode::kOk, "dispatch-accepted");
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
    case MessageFamily::kSystemService:
        handle_system_service_envelope(ev);
        return;
    case MessageFamily::kConfigurationService:
        handle_configuration_service_envelope(ev);
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
