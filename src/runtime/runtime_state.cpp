/**
 * @file src/runtime/runtime_state.cpp
 * @brief Runtime 状态、事件与结果订阅实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

StateSubscriber::StateSubscriber(Runtime* runtime) : runtime_(runtime) {}

void StateSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t StateSubscriber::subscribe_vehicle_core(VehicleCoreHandler cb) {
    return runtime_ ? runtime_->subscribe_vehicle_core_internal(std::move(cb)) : 0;
}

void StateSubscriber::unsubscribe(size_t token) {
    if (runtime_) {
        runtime_->unsubscribe_semantic(token);
    }
}

EventSubscriber::EventSubscriber(Runtime* runtime) : runtime_(runtime) {}

void EventSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t EventSubscriber::subscribe_vehicle_event(VehicleEventHandler cb) {
    return runtime_ ? runtime_->subscribe_vehicle_event_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_command_results(CommandResultHandler cb) {
    return runtime_ ? runtime_->subscribe_command_result_internal(std::move(cb)) : 0;
}

void EventSubscriber::unsubscribe(size_t token) {
    if (runtime_) {
        runtime_->unsubscribe_semantic(token);
    }
}

size_t Runtime::subscribe_vehicle_core_internal(StateSubscriber::VehicleCoreHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->vehicle_core_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_vehicle_event_internal(EventSubscriber::VehicleEventHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->vehicle_event_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_command_result_internal(EventSubscriber::CommandResultHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->command_result_handlers[token] = std::move(cb);
    return token;
}

void Runtime::unsubscribe_semantic(size_t token) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->vehicle_core_handlers.erase(token);
    impl_->vehicle_event_handlers.erase(token);
    impl_->command_result_handlers.erase(token);
}

ErrorCode Runtime::publish_vehicle_core_state(const std::string& peer_id,
                                              const TargetSelector& target,
                                              const VehicleCoreState& payload,
                                              uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_vehicle_event(const std::string& peer_id,
                                         const TargetSelector& target,
                                         const VehicleEvent& payload,
                                         uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kBestEffort, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

void Runtime::handle_state_snapshot_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity) ||
        ev.envelope.message_type != static_cast<uint16_t>(StateSnapshotType::kVehicleCore)) {
        return;
    }

    VehicleCoreState payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        return;
    }

    TypedMessage<VehicleCoreState> message{ev.envelope, payload};
    std::unordered_map<size_t, StateSubscriber::VehicleCoreHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        handlers = impl_->vehicle_core_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(message);
        }
    }
}

void Runtime::handle_state_event_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity) ||
        ev.envelope.message_type != static_cast<uint16_t>(StateEventType::kVehicleEvent)) {
        return;
    }

    VehicleEvent payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        return;
    }

    TypedMessage<VehicleEvent> message{ev.envelope, payload};
    std::unordered_map<size_t, EventSubscriber::VehicleEventHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        handlers = impl_->vehicle_event_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(message);
        }
    }
}

void Runtime::handle_command_result_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity)) {
        return;
    }

    CommandResult payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        return;
    }

    CommandResultView view{ev.envelope, payload};
    std::unordered_map<size_t, EventSubscriber::CommandResultHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        handlers = impl_->command_result_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(view);
        }
    }
}

}  // namespace sunraycom
