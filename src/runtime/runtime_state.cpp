/**
 * @file src/runtime/runtime_state.cpp
 * @brief Runtime 状态、事件与结果订阅实现。
 */

#include "runtime_internal.hpp"

namespace yunlink {

namespace {

template <typename Payload, typename HandlerMap>
bool fanout_snapshot(std::mutex& mu,
                     const SecureEnvelope& envelope,
                     const ByteBuffer& bytes,
                     const HandlerMap& source_handlers) {
    Payload payload{};
    if (!decode_typed_payload(bytes, &payload)) {
        return false;
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
    return true;
}

void publish_semantic_decode_error(EventBus& bus, const EnvelopeEvent& ev) {
    ErrorEvent error;
    error.code = ErrorCode::kDecodeError;
    error.transport = ev.transport;
    error.peer = ev.peer;
    error.message = "semantic-payload-decode-failed";
    bus.publish_error(error);
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

size_t EventSubscriber::subscribe_authority_status(AuthorityStatusHandler cb) {
    return runtime_ ? runtime_->subscribe_authority_status_internal(std::move(cb)) : 0;
}

size_t EventSubscriber::subscribe_bulk_channel_descriptors(BulkChannelDescriptorHandler cb) {
    return runtime_ ? runtime_->subscribe_bulk_channel_descriptor_internal(std::move(cb)) : 0;
}

void EventSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

CommandSubscriber::CommandSubscriber(Runtime* runtime) : runtime_(runtime) {}
void CommandSubscriber::bind(Runtime* runtime) {
    runtime_ = runtime;
}

size_t CommandSubscriber::subscribe_takeoff(TakeoffHandler cb) {
    return runtime_ ? runtime_->subscribe_takeoff_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_land(LandHandler cb) {
    return runtime_ ? runtime_->subscribe_land_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_return(ReturnHandler cb) {
    return runtime_ ? runtime_->subscribe_return_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_goto(GotoHandler cb) {
    return runtime_ ? runtime_->subscribe_goto_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_velocity_setpoint(VelocitySetpointHandler cb) {
    return runtime_ ? runtime_->subscribe_velocity_setpoint_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_trajectory_chunk(TrajectoryChunkHandler cb) {
    return runtime_ ? runtime_->subscribe_trajectory_chunk_internal(std::move(cb)) : 0;
}

size_t CommandSubscriber::subscribe_formation_task(FormationTaskHandler cb) {
    return runtime_ ? runtime_->subscribe_formation_task_internal(std::move(cb)) : 0;
}

void CommandSubscriber::unsubscribe(size_t token) {
    if (runtime_)
        runtime_->unsubscribe_semantic(token);
}

size_t Runtime::subscribe_takeoff_internal(CommandSubscriber::TakeoffHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->takeoff_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_land_internal(CommandSubscriber::LandHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->land_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_return_internal(CommandSubscriber::ReturnHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->return_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_goto_internal(CommandSubscriber::GotoHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->goto_handlers[token] = std::move(cb);
    return token;
}

size_t
Runtime::subscribe_velocity_setpoint_internal(CommandSubscriber::VelocitySetpointHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->velocity_setpoint_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_trajectory_chunk_internal(CommandSubscriber::TrajectoryChunkHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->trajectory_chunk_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_formation_task_internal(CommandSubscriber::FormationTaskHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->formation_task_handlers[token] = std::move(cb);
    return token;
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

size_t Runtime::subscribe_authority_status_internal(EventSubscriber::AuthorityStatusHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->authority_status_handlers[token] = std::move(cb);
    return token;
}

size_t Runtime::subscribe_bulk_channel_descriptor_internal(
    EventSubscriber::BulkChannelDescriptorHandler cb) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const size_t token = impl_->next_token++;
    impl_->bulk_channel_descriptor_handlers[token] = std::move(cb);
    return token;
}

void Runtime::unsubscribe_semantic(size_t token) {
    std::lock_guard<std::mutex> lock(impl_->mu);
    impl_->takeoff_handlers.erase(token);
    impl_->land_handlers.erase(token);
    impl_->return_handlers.erase(token);
    impl_->goto_handlers.erase(token);
    impl_->velocity_setpoint_handlers.erase(token);
    impl_->trajectory_chunk_handlers.erase(token);
    impl_->formation_task_handlers.erase(token);
    impl_->vehicle_core_handlers.erase(token);
    impl_->px4_state_handlers.erase(token);
    impl_->odom_status_handlers.erase(token);
    impl_->uav_control_fsm_state_handlers.erase(token);
    impl_->uav_controller_state_handlers.erase(token);
    impl_->gimbal_params_handlers.erase(token);
    impl_->vehicle_event_handlers.erase(token);
    impl_->command_result_handlers.erase(token);
    impl_->authority_status_handlers.erase(token);
    impl_->bulk_channel_descriptor_handlers.erase(token);
}

void Runtime::handle_state_snapshot_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity)) {
        return;
    }

    if (ev.envelope.qos_class == QosClass::kReliableLatest) {
        std::lock_guard<std::mutex> lock(impl_->mu);
        const std::string key = runtime_qos_latest_key(ev.envelope);
        const auto it = impl_->reliable_latest_watermarks.find(key);
        if (it != impl_->reliable_latest_watermarks.end() &&
            ev.envelope.message_id <= it->second) {
            return;
        }
        impl_->reliable_latest_watermarks[key] = ev.envelope.message_id;
    }

    switch (static_cast<StateSnapshotType>(ev.envelope.message_type)) {
    case StateSnapshotType::kVehicleCore:
        if (!fanout_snapshot<VehicleCoreState>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->vehicle_core_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kPx4State:
        if (!fanout_snapshot<Px4StateSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->px4_state_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kOdomStatus:
        if (!fanout_snapshot<OdomStatusSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->odom_status_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControlFsmState:
        if (!fanout_snapshot<UavControlFsmStateSnapshot>(impl_->mu,
                                                         ev.envelope,
                                                         ev.envelope.payload,
                                                         impl_->uav_control_fsm_state_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControllerState:
        if (!fanout_snapshot<UavControllerStateSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->uav_controller_state_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kGimbalParams:
        if (!fanout_snapshot<GimbalParamsSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->gimbal_params_handlers)) {
            publish_semantic_decode_error(bus_, ev);
        }
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
        publish_semantic_decode_error(bus_, ev);
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
        publish_semantic_decode_error(bus_, ev);
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

void Runtime::handle_bulk_channel_descriptor_envelope(const EnvelopeEvent& ev) {
    if (!ev.envelope.target.matches(config_.self_identity) ||
        ev.envelope.message_type != static_cast<uint16_t>(BulkDescriptorType::kDescriptor)) {
        return;
    }

    if (ev.envelope.qos_class != QosClass::kReliableOrdered) {
        ErrorEvent error;
        error.code = ErrorCode::kRejected;
        error.transport = ev.transport;
        error.peer = ev.peer;
        error.message = "bulk-descriptor-qos-requires-reliable-ordered";
        bus_.publish_error(error);
        return;
    }

    BulkChannelDescriptor payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        publish_semantic_decode_error(bus_, ev);
        return;
    }

    TypedMessage<BulkChannelDescriptor> message{ev.envelope, payload};
    std::unordered_map<size_t, EventSubscriber::BulkChannelDescriptorHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        if (payload.channel_id != 0 && payload.state == BulkChannelState::kReady &&
            !payload.uri.empty()) {
            impl_->active_bulk_channels[payload.channel_id] = payload;
        } else if (payload.channel_id != 0) {
            impl_->active_bulk_channels.erase(payload.channel_id);
        }
        handlers = impl_->bulk_channel_descriptor_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(message);
        }
    }
}

bool Runtime::current_bulk_channel(uint32_t channel_id, BulkChannelDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->active_bulk_channels.find(channel_id);
    if (it == impl_->active_bulk_channels.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second;
    }
    return true;
}

}  // namespace yunlink
