/**
 * @file src/runtime/command/command.cpp
 * @brief Runtime command publishing implementation.
 */

#include "../core/internal.hpp"

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

ErrorCode CommandPublisher::publish_uav_control(const std::string& peer_id,
                                                uint64_t session_id,
                                                const TargetSelector& target,
                                                const UavControlCommand& payload,
                                                CommandHandle* out_handle) {
    return runtime_ == nullptr ? ErrorCode::kInvalidArgument
                               : runtime_->publish_command_payload(
                                     peer_id,
                                     session_id,
                                     target,
                                     MessageTraits<UavControlCommand>::kMessageType,
                                     encode_payload(payload),
                                     out_handle,
                                     1000);
}

}  // namespace yunlink
