/**
 * @file src/runtime/runtime_state.cpp
 * @brief Runtime 状态、事件与结果订阅实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

namespace {

template <typename Payload, typename HandlerMap>
void fanout_snapshot(std::mutex& mu,
                     const SecureEnvelope& envelope,
                     const ByteBuffer& bytes,
                     const HandlerMap& source_handlers) {
    Payload payload{};
    if (!decode_typed_payload(bytes, &payload)) {
        return;
    }
    TypedMessage<Payload> message{envelope, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(message);
        }
    }
}

}  // namespace

StateSubscriber::StateSubscriber(Runtime* runtime) : runtime_(runtime) {}
void StateSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t StateSubscriber::subscribe_vehicle_core(VehicleCoreHandler cb) {
    return runtime_ ? runtime_->subscribe_vehicle_core_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_px4_state(Px4StateHandler cb) {
    return runtime_ ? runtime_->subscribe_px4_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_odom_status(OdomStatusHandler cb) {
    return runtime_ ? runtime_->subscribe_odom_status_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_control_fsm_state(UavControlFsmStateHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_control_fsm_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_uav_controller_state(UavControllerStateHandler cb) {
    return runtime_ ? runtime_->subscribe_uav_controller_state_internal(std::move(cb)) : 0;
}

size_t StateSubscriber::subscribe_gimbal_params(GimbalParamsHandler cb) {
    return runtime_ ? runtime_->subscribe_gimbal_params_internal(std::move(cb)) : 0;
}

void StateSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
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
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

size_t Runtime::subscribe_vehicle_core_internal(StateSubscriber::VehicleCoreHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->vehicle_core_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_px4_state_internal(StateSubscriber::Px4StateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->px4_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_odom_status_internal(StateSubscriber::OdomStatusHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->odom_status_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_uav_control_fsm_state_internal(StateSubscriber::UavControlFsmStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_control_fsm_state_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_uav_controller_state_internal(StateSubscriber::UavControllerStateHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->uav_controller_state_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_gimbal_params_internal(StateSubscriber::GimbalParamsHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->gimbal_params_handlers[token] = std::move(cb);
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
    impl_->px4_state_handlers.erase(token);
    impl_->odom_status_handlers.erase(token);
    impl_->uav_control_fsm_state_handlers.erase(token);
    impl_->uav_controller_state_handlers.erase(token);
    impl_->gimbal_params_handlers.erase(token);
    impl_->vehicle_event_handlers.erase(token);
    impl_->command_result_handlers.erase(token);
}

void Runtime::handle_state_snapshot_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity)) {
        return;
    }

    switch (static_cast<StateSnapshotType>(ev.envelope.message_type)) {
    case StateSnapshotType::kVehicleCore:
        fanout_snapshot<VehicleCoreState>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->vehicle_core_handlers);
        return;
    case StateSnapshotType::kPx4State:
        fanout_snapshot<Px4StateSnapshot>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->px4_state_handlers);
        return;
    case StateSnapshotType::kOdomStatus:
        fanout_snapshot<OdomStatusSnapshot>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->odom_status_handlers);
        return;
    case StateSnapshotType::kUavControlFsmState:
        fanout_snapshot<UavControlFsmStateSnapshot>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->uav_control_fsm_state_handlers);
        return;
    case StateSnapshotType::kUavControllerState:
        fanout_snapshot<UavControllerStateSnapshot>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->uav_controller_state_handlers);
        return;
    case StateSnapshotType::kGimbalParams:
        fanout_snapshot<GimbalParamsSnapshot>(
            impl_->mu, ev.envelope, ev.envelope.payload, impl_->gimbal_params_handlers);
        return;
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
