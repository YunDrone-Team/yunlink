/**
 * @file src/runtime/command/receive.cpp
 * @brief Runtime inbound command validation and dispatch.
 */

#include "fanout.hpp"

namespace yunlink {
namespace {

CommandResult make_command_result(const EnvelopeEvent& ev,
                                  ErrorCode code,
                                  CommandPhase phase,
                                  uint8_t progress_percent,
                                  const std::string& detail) {
    CommandResult result{};
    result.command_kind = runtime_command_kind_for_message_type(ev.envelope.message_type);
    result.phase = phase;
    result.result_code = static_cast<uint16_t>(code);
    result.progress_percent = progress_percent;
    result.detail = detail;
    return result;
}

}  // namespace

void Runtime::handle_command_envelope(const EnvelopeEvent& ev) {
    const auto fail_command = [&](ErrorCode code, const std::string& detail) {
        (void)reply_command_result(ev,
                                   make_command_result(ev, code, CommandPhase::kFailed, 0, detail));
    };

    const auto ack_command =
        [&](CommandPhase phase, uint8_t progress_percent, const std::string& detail) {
            (void)reply_command_result(
                ev, make_command_result(ev, ErrorCode::kOk, phase, progress_percent, detail));
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
            runtime_fanout_command_payload<TrajectoryChunkCommand>(
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
            return runtime_fanout_command<TakeoffCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->takeoff_handlers);
        case CommandType::kLand:
            return runtime_fanout_command<LandCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->land_handlers);
        case CommandType::kReturn:
            return runtime_fanout_command<ReturnCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->return_handlers);
        case CommandType::kGoto:
            return runtime_fanout_command<GotoCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->goto_handlers);
        case CommandType::kVelocitySetpoint:
            return runtime_fanout_command<VelocitySetpointCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->velocity_setpoint_handlers);
        case CommandType::kTrajectoryChunk:
            return runtime_fanout_command<TrajectoryChunkCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->trajectory_chunk_handlers);
        case CommandType::kFormationTask:
            return runtime_fanout_command<FormationTaskCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->formation_task_handlers);
        case CommandType::kUavControl:
            return runtime_fanout_command<UavControlCommand>(
                impl_->mu, ev, ev.envelope.payload, impl_->uav_control_handlers);
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
