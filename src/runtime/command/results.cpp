/**
 * @file src/runtime/command/results.cpp
 * @brief Runtime command result reply helpers.
 */

#include "../core/internal.hpp"

namespace yunlink {

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

}  // namespace yunlink
