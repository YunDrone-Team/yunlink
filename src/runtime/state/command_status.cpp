/**
 * @file src/runtime/state/command_status.cpp
 * @brief Runtime command execution status handling.
 */

#include "fanout.hpp"

namespace yunlink {

namespace {

bool runtime_command_phase_from_execution_status(const CommandExecutionStatusSnapshot& status,
                                                 CommandPhase* out) {
    if (out == nullptr) {
        return false;
    }

    switch (static_cast<CommandExecutionState>(status.execution_state)) {
    case CommandExecutionState::kSucceeded:
        *out = CommandPhase::kSucceeded;
        return true;
    case CommandExecutionState::kFailed:
        *out = CommandPhase::kFailed;
        return true;
    case CommandExecutionState::kCancelled:
        *out = CommandPhase::kCancelled;
        return true;
    case CommandExecutionState::kTimeout:
        *out = CommandPhase::kExpired;
        return true;
    case CommandExecutionState::kIdle:
    case CommandExecutionState::kAccepted:
    case CommandExecutionState::kRunning:
    case CommandExecutionState::kWaitingPhysicalState:
        break;
    }

    if (!status.terminal) {
        return false;
    }
    *out = status.success ? CommandPhase::kSucceeded : CommandPhase::kFailed;
    return true;
}

std::string runtime_command_status_identity_key(const EnvelopeEvent& ev,
                                                const CommandExecutionStatusSnapshot& status) {
    const uint64_t session_id = status.session_id != 0 ? status.session_id : ev.envelope.session_id;
    if (session_id == 0) {
        return {};
    }

    const uint64_t command_id = status.command_correlation_id != 0 ? status.command_correlation_id
                                                                   : status.command_message_id;
    if (command_id == 0) {
        return {};
    }

    std::string peer = ev.peer.id;
    if (peer.empty()) {
        peer = std::to_string(static_cast<uint8_t>(ev.envelope.source.agent_type)) + ":" +
               std::to_string(ev.envelope.source.agent_id);
    }

    const char* id_kind = status.command_correlation_id != 0 ? "corr" : "msg";
    return peer + "#" + std::to_string(static_cast<uint8_t>(ev.envelope.source.agent_type)) + ":" +
           std::to_string(ev.envelope.source.agent_id) + "#" + std::to_string(session_id) + "#" +
           id_kind + ":" + std::to_string(command_id);
}

}  // namespace

void Runtime::handle_command_execution_status_snapshot(const EnvelopeEvent& ev) {
    CommandExecutionStatusSnapshot status{};
    if (!decode_typed_payload(ev.envelope.payload, &status)) {
        runtime_publish_semantic_decode_error(bus_, ev);
        return;
    }

    const TypedMessage<CommandExecutionStatusSnapshot> status_message{ev.envelope, status};
    std::unordered_map<size_t, StateSubscriber::CommandExecutionStatusHandler> status_handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        status_handlers = impl_->command_execution_status_handlers;
    }
    for (const auto& item : status_handlers) {
        if (item.second) {
            item.second(status_message);
        }
    }

    CommandPhase phase = CommandPhase::kReceived;
    if (status.command_kind == CommandKind::kUnknown ||
        !runtime_command_phase_from_execution_status(status, &phase)) {
        return;
    }

    const std::string identity_key = runtime_command_status_identity_key(ev, status);
    if (identity_key.empty()) {
        return;
    }

    std::unordered_map<size_t, EventSubscriber::CommandResultHandler> result_handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        if (impl_->command_result_from_status_seen.find(identity_key) !=
            impl_->command_result_from_status_seen.end()) {
            return;
        }
        impl_->command_result_from_status_seen[identity_key] = runtime_now_millis();
        result_handlers = impl_->command_result_handlers;
    }

    CommandResult result{};
    result.command_kind = status.command_kind;
    result.phase = phase;
    result.result_code = status.result_code;
    result.progress_percent = status.progress_percent;
    result.detail = status.detail;

    const uint64_t session_id = status.session_id != 0 ? status.session_id : ev.envelope.session_id;
    const uint64_t correlation_id = status.command_correlation_id != 0
                                        ? status.command_correlation_id
                                        : status.command_message_id;
    SecureEnvelope result_envelope =
        make_typed_envelope(ev.envelope.source,
                            TargetSelector::for_entity(config_.self_identity.agent_type,
                                                       config_.self_identity.agent_id),
                            session_id,
                            correlation_id,
                            QosClass::kReliableOrdered,
                            result,
                            ev.envelope.ttl_ms);
    result_envelope.message_id = allocate_message_id();
    result_envelope.correlation_id = correlation_id;

    const CommandResultView result_view{result_envelope, result};
    for (const auto& item : result_handlers) {
        if (item.second) {
            item.second(result_view);
        }
    }
}

}  // namespace yunlink
