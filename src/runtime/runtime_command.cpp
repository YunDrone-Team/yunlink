/**
 * @file src/runtime/runtime_command.cpp
 * @brief Runtime 命令发布与结果反馈实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

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
    SessionDescriptor session{};
    if (!describe_session_internal(ev.envelope.session_id, &session) ||
        session.state != SessionState::kActive ||
        !ev.envelope.target.matches(config_.self_identity)) {
        return;
    }

    AuthorityLease lease{};
    if (!current_authority(&lease) || lease.session_id != ev.envelope.session_id) {
        return;
    }

    publish_command_result_sequence(ev, ev.envelope);
}

}  // namespace sunraycom
