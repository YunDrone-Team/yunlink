/**
 * @file src/runtime/runtime_command.cpp
 * @brief Runtime 命令发布与结果反馈实现。
 */

#include "runtime_internal.hpp"

namespace yunlink {

namespace {

void fill_command_handle(CommandHandle* out_handle,
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

template <typename Payload, typename HandlerMap>
bool fanout_command(std::mutex& mu,
                    const EnvelopeEvent& ev,
                    const ByteBuffer& bytes,
                    const HandlerMap& source_handlers) {
    Payload payload{};
    if (!decode_typed_payload(bytes, &payload)) {
        return false;
    }

    InboundCommandView<Payload> view{ev, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(view);
        }
    }
    return true;
}

template <typename Payload, typename HandlerMap>
void fanout_command_payload(std::mutex& mu,
                            const EnvelopeEvent& ev,
                            const Payload& payload,
                            const HandlerMap& source_handlers) {
    InboundCommandView<Payload> view{ev, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(view);
        }
    }
}

}  // namespace

CommandPublisher::CommandPublisher(Runtime* runtime) : runtime_(runtime) {}

void CommandPublisher::bind(Runtime* runtime) {
    runtime_ = runtime;
}

ErrorCode Runtime::publish_command_payload(const std::string& peer_id,
                                           uint64_t session_id,
                                           const TargetSelector& target,
                                           uint16_t message_type,
                                           const ByteBuffer& payload,
                                           CommandHandle* out_handle,
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
                                                    MessageFamily::kCommand,
                                                    message_type,
                                                    payload,
                                                    ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = envelope.message_id;

    const ErrorCode ec = send_envelope_to_peer(peer_id, envelope);
    if (ec == ErrorCode::kOk) {
        fill_command_handle(
            out_handle, session_id, envelope.message_id, envelope.correlation_id, target);
    }
    return ec;
}

ErrorCode CommandPublisher::publish_takeoff(const std::string& peer_id,
                                            uint64_t session_id,
                                            const TargetSelector& target,
                                            const TakeoffCommand& payload,
                                            CommandHandle* out_handle) {
    return runtime_ == nullptr
               ? ErrorCode::kInvalidArgument
               : runtime_->publish_command_payload(peer_id,
                                                   session_id,
                                                   target,
                                                   MessageTraits<TakeoffCommand>::kMessageType,
                                                   encode_payload(payload),
                                                   out_handle,
                                                   1000);
}

ErrorCode CommandPublisher::publish_land(const std::string& peer_id,
                                         uint64_t session_id,
                                         const TargetSelector& target,
                                         const LandCommand& payload,
                                         CommandHandle* out_handle) {
    return runtime_ == nullptr
               ? ErrorCode::kInvalidArgument
               : runtime_->publish_command_payload(peer_id,
                                                   session_id,
                                                   target,
                                                   MessageTraits<LandCommand>::kMessageType,
                                                   encode_payload(payload),
                                                   out_handle,
                                                   1000);
}

ErrorCode CommandPublisher::publish_return(const std::string& peer_id,
                                           uint64_t session_id,
                                           const TargetSelector& target,
                                           const ReturnCommand& payload,
                                           CommandHandle* out_handle) {
    return runtime_ == nullptr
               ? ErrorCode::kInvalidArgument
               : runtime_->publish_command_payload(peer_id,
                                                   session_id,
                                                   target,
                                                   MessageTraits<ReturnCommand>::kMessageType,
                                                   encode_payload(payload),
                                                   out_handle,
                                                   1000);
}

ErrorCode CommandPublisher::publish_goto(const std::string& peer_id,
                                         uint64_t session_id,
                                         const TargetSelector& target,
                                         const GotoCommand& payload,
                                         CommandHandle* out_handle) {
    return runtime_ == nullptr
               ? ErrorCode::kInvalidArgument
               : runtime_->publish_command_payload(peer_id,
                                                   session_id,
                                                   target,
                                                   MessageTraits<GotoCommand>::kMessageType,
                                                   encode_payload(payload),
                                                   out_handle,
                                                   1000);
}

ErrorCode CommandPublisher::publish_velocity_setpoint(const std::string& peer_id,
                                                      uint64_t session_id,
                                                      const TargetSelector& target,
                                                      const VelocitySetpointCommand& payload,
                                                      CommandHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_command_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<VelocitySetpointCommand>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     1000);
}

ErrorCode CommandPublisher::publish_trajectory_chunk(const std::string& peer_id,
                                                     uint64_t session_id,
                                                     const TargetSelector& target,
                                                     const TrajectoryChunkCommand& payload,
                                                     CommandHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_command_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<TrajectoryChunkCommand>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     1000);
}

ErrorCode CommandPublisher::publish_formation_task(const std::string& peer_id,
                                                   uint64_t session_id,
                                                   const TargetSelector& target,
                                                   const FormationTaskCommand& payload,
                                                   CommandHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_command_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<FormationTaskCommand>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     1000);
}

ErrorCode Runtime::reply_command_result(const EnvelopeEvent& inbound,
                                        const CommandResult& payload,
                                        uint32_t ttl_ms) {
    CommandResult routed = payload;
    if (routed.command_kind == CommandKind::kUnknown) {
        routed.command_kind = runtime_command_kind_for_message_type(inbound.envelope.message_type);
    }

    SecureEnvelope envelope =
        make_typed_envelope(config_.self_identity,
                            TargetSelector::for_entity(inbound.envelope.source.agent_type,
                                                       inbound.envelope.source.agent_id),
                            inbound.envelope.session_id,
                            inbound.envelope.message_id,
                            QosClass::kReliableOrdered,
                            routed,
                            ttl_ms);
    envelope.message_id = allocate_message_id();
    envelope.correlation_id = inbound.envelope.message_id;
    return reply_on_route(inbound, envelope);
}

void Runtime::publish_command_result_sequence(const EnvelopeEvent& inbound,
                                              const SecureEnvelope& cmd) {
    const CommandKind command_kind = runtime_command_kind_for_message_type(cmd.message_type);
    const CommandPhase phases[] = {
        CommandPhase::kReceived,
        CommandPhase::kAccepted,
        CommandPhase::kInProgress,
        CommandPhase::kSucceeded,
    };

    for (size_t i = 0; i < 4; ++i) {
        CommandResult result{};
        result.command_kind = command_kind;
        result.phase = phases[i];
        result.progress_percent =
            static_cast<uint8_t>(i == 0 ? 0 : (i == 1 ? 10 : (i == 2 ? 60 : 100)));
        result.detail = "runtime-auto-result";

        SecureEnvelope envelope = make_typed_envelope(
            config_.self_identity,
            TargetSelector::for_entity(cmd.source.agent_type, cmd.source.agent_id),
            cmd.session_id,
            cmd.message_id,
            QosClass::kReliableOrdered,
            result,
            1000);
        envelope.message_id = allocate_message_id();
        envelope.correlation_id = cmd.message_id;
        reply_on_route(inbound, envelope);
    }
}

void Runtime::handle_command_envelope(const EnvelopeEvent& ev) {
    const auto fail_command = [&](ErrorCode code, const std::string& detail) {
        CommandResult result{};
        result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
        result.phase = CommandPhase::kFailed;
        result.result_code = static_cast<uint16_t>(code);
        result.progress_percent = 0;
        result.detail = detail;
        (void)reply_command_result(ev, result);
    };

    const auto ack_command =
        [&](CommandPhase phase, uint8_t progress_percent, const std::string& detail) {
            CommandResult result{};
            result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
            result.phase = phase;
            result.result_code = static_cast<uint16_t>(ErrorCode::kOk);
            result.progress_percent = progress_percent;
            result.detail = detail;
            (void)reply_command_result(ev, result);
        };

    if (ev.envelope.target.scope == TargetScope::kBroadcast) {
        fail_command(ErrorCode::kRejected, "broadcast-command-disallowed");
        return;
    }

    if (ev.envelope.qos_class != QosClass::kReliableOrdered) {
        fail_command(ErrorCode::kRejected, "command-qos-requires-reliable-ordered");
        return;
    }

    if (!ev.envelope.target.matches(config_.self_identity)) {
        fail_command(ErrorCode::kRejected, "wrong-target");
        return;
    }

    SessionDescriptor session{};
    if (!describe_session_internal(ev.peer.id, ev.envelope.session_id, &session)) {
        fail_command(ErrorCode::kRejected, "no-active-session");
        return;
    }
    if (session.state == SessionState::kLost) {
        fail_command(ErrorCode::kRejected, "session-lost");
        return;
    }
    if (session.state != SessionState::kActive) {
        fail_command(ErrorCode::kRejected, "no-active-session");
        return;
    }

    if (static_cast<CommandType>(ev.envelope.message_type) == CommandType::kFormationTask) {
        FormationTaskCommand formation{};
        if (!decode_typed_payload(ev.envelope.payload, &formation)) {
            fail_command(ErrorCode::kDecodeError, "formation-decode-failed");
            return;
        }
        if (ev.envelope.target.scope != TargetScope::kGroup) {
            fail_command(ErrorCode::kRejected, "formation-target-not-group");
            return;
        }
        if (formation.group_id != ev.envelope.target.group_id) {
            fail_command(ErrorCode::kRejected, "formation-group-mismatch");
            return;
        }
    }

    AuthorityLease lease{};
    if (!current_authority_for_target(ev.envelope.target, &lease) ||
        lease.session_id != ev.envelope.session_id || lease.peer.id != ev.peer.id) {
        fail_command(ErrorCode::kUnauthorized, "no-authority");
        return;
    }

    if (static_cast<CommandType>(ev.envelope.message_type) == CommandType::kTrajectoryChunk) {
        TrajectoryChunkCommand chunk{};
        if (!decode_typed_payload(ev.envelope.payload, &chunk)) {
            fail_command(ErrorCode::kDecodeError, "trajectory-decode-failed");
            return;
        }

        bool should_ack_buffered = false;
        bool should_dispatch = false;
        TrajectoryChunkCommand assembled{};
        std::string failure_detail;
        ErrorCode failure_code = ErrorCode::kRejected;
        const uint64_t now_ms = runtime_now_millis();

        {
            std::lock_guard<std::mutex> lock(impl_->mu);
            const std::string key = runtime_trajectory_key(ev);
            auto it = impl_->trajectory_accumulators.find(key);
            if (it != impl_->trajectory_accumulators.end() &&
                config_.trajectory_chunk_timeout_ms > 0 && now_ms > it->second.updated_at_ms &&
                now_ms - it->second.updated_at_ms > config_.trajectory_chunk_timeout_ms) {
                impl_->trajectory_accumulators.erase(it);
                it = impl_->trajectory_accumulators.end();
                failure_detail = "trajectory-chunk-timeout";
                failure_code = ErrorCode::kTimeout;
            } else if (it == impl_->trajectory_accumulators.end()) {
                if (chunk.chunk_index != 0) {
                    failure_detail = "trajectory-missing-chunk";
                } else {
                    RuntimeTrajectoryAccumulator accumulator{};
                    accumulator.next_chunk_index = 1;
                    accumulator.updated_at_ms = now_ms;
                    accumulator.assembled.chunk_index = 0;
                    accumulator.assembled.final_chunk = chunk.final_chunk;
                    accumulator.assembled.points = chunk.points;
                    if (chunk.final_chunk) {
                        assembled = accumulator.assembled;
                        should_dispatch = true;
                    } else {
                        impl_->trajectory_accumulators.emplace(key, accumulator);
                        should_ack_buffered = true;
                    }
                }
            } else if (chunk.chunk_index < it->second.next_chunk_index) {
                failure_detail = "trajectory-duplicate-chunk";
            } else if (chunk.chunk_index > it->second.next_chunk_index) {
                impl_->trajectory_accumulators.erase(it);
                failure_detail = "trajectory-missing-chunk";
            } else {
                it->second.assembled.points.insert(
                    it->second.assembled.points.end(), chunk.points.begin(), chunk.points.end());
                it->second.assembled.final_chunk = chunk.final_chunk;
                it->second.next_chunk_index = chunk.chunk_index + 1;
                it->second.updated_at_ms = now_ms;
                if (chunk.final_chunk) {
                    assembled = it->second.assembled;
                    impl_->trajectory_accumulators.erase(it);
                    should_dispatch = true;
                } else {
                    should_ack_buffered = true;
                }
            }
        }

        if (!failure_detail.empty()) {
            fail_command(failure_code, failure_detail);
            return;
        }
        if (should_ack_buffered) {
            ack_command(CommandPhase::kAccepted, 10, "trajectory-chunk-buffered");
            return;
        }
        if (should_dispatch) {
            EnvelopeEvent assembled_ev = ev;
            assembled_ev.envelope.payload = encode_payload(assembled);
            assembled_ev.envelope.payload_len =
                static_cast<uint32_t>(assembled_ev.envelope.payload.size());
            fanout_command_payload<TrajectoryChunkCommand>(
                impl_->mu, assembled_ev, assembled, impl_->trajectory_chunk_handlers);
            if (config_.command_handling_mode != CommandHandlingMode::kExternalHandler) {
                publish_command_result_sequence(assembled_ev, assembled_ev.envelope);
            }
            return;
        }
    }

    const auto dispatch_command = [&]() -> bool {
        switch (static_cast<CommandType>(ev.envelope.message_type)) {
        case CommandType::kTakeoff:
            return fanout_command<TakeoffCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->takeoff_handlers);
        case CommandType::kLand:
            return fanout_command<LandCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->land_handlers);
        case CommandType::kReturn:
            return fanout_command<ReturnCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->return_handlers);
        case CommandType::kGoto:
            return fanout_command<GotoCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->goto_handlers);
        case CommandType::kVelocitySetpoint:
            return fanout_command<VelocitySetpointCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->velocity_setpoint_handlers);
        case CommandType::kTrajectoryChunk:
            return fanout_command<TrajectoryChunkCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->trajectory_chunk_handlers);
        case CommandType::kFormationTask:
            return fanout_command<FormationTaskCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->formation_task_handlers);
        }
        return false;
    };

    if (config_.command_handling_mode == CommandHandlingMode::kExternalHandler) {
        if (!dispatch_command()) {
            fail_command(ErrorCode::kDecodeError, "semantic-payload-decode-failed");
        }
        return;
    }

    if (!dispatch_command()) {
        fail_command(ErrorCode::kDecodeError, "semantic-payload-decode-failed");
        return;
    }
    publish_command_result_sequence(ev, ev.envelope);
}

}  // namespace yunlink
