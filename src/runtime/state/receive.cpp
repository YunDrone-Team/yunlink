/**
 * @file src/runtime/state/receive.cpp
 * @brief Runtime inbound state and semantic event dispatch.
 */

#include "fanout.hpp"

namespace yunlink {

void Runtime::handle_state_snapshot_envelope(const EnvelopeEvent& ev) {
    if (!matches_local_target(ev.envelope.target)) {
        return;
    }

    if (ev.envelope.qos_class == QosClass::kReliableLatest) {
        std::lock_guard<std::mutex> lock(impl_->mu);
        const std::string key = runtime_qos_latest_key(ev.envelope);
        const auto it = impl_->reliable_latest_watermarks.find(key);
        if (it != impl_->reliable_latest_watermarks.end() && ev.envelope.message_id <= it->second) {
            return;
        }
        impl_->reliable_latest_watermarks[key] = ev.envelope.message_id;
    }

    switch (static_cast<StateSnapshotType>(ev.envelope.message_type)) {
    case StateSnapshotType::kVehicleCore:
        if (!runtime_fanout_snapshot<VehicleCoreState>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->vehicle_core_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kPx4State:
        if (!runtime_fanout_snapshot<Px4StateSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->px4_state_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kOdomStatus:
        if (!runtime_fanout_snapshot<OdomStatusSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->odom_status_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControlFsmState:
        if (!runtime_fanout_snapshot<UavControlFsmStateSnapshot>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->uav_control_fsm_state_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControllerState:
        if (!runtime_fanout_snapshot<UavControllerStateSnapshot>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->uav_controller_state_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kGimbalParams:
        if (!runtime_fanout_snapshot<GimbalParamsSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->gimbal_params_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kLocalOdom:
        if (!runtime_fanout_snapshot<LocalOdomSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->local_odom_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControlCmd:
        if (!runtime_fanout_snapshot<UavControlCmdSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->uav_control_cmd_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kUavControlState:
        if (!runtime_fanout_snapshot<UavControlStateSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->uav_control_state_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kCommandExecutionStatus:
        handle_command_execution_status_snapshot(ev);
        return;
    case StateSnapshotType::kOdomState:
        if (!runtime_fanout_snapshot<OdomStateSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->odom_state_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kSunrayRuntimeDiagnostic:
        if (!runtime_fanout_snapshot<SunrayRuntimeDiagnosticSnapshot>(
                impl_->mu,
                ev.envelope,
                ev.envelope.payload,
                impl_->sunray_runtime_diagnostic_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kHostSystem:
        if (!runtime_fanout_snapshot<HostSystemSnapshot>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->host_system_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    case StateSnapshotType::kTopicSample:
        if (!runtime_fanout_snapshot<TopicSample>(
                impl_->mu, ev.envelope, ev.envelope.payload, impl_->topic_sample_handlers)) {
            runtime_publish_semantic_decode_error(bus_, ev);
        }
        return;
    }
}

void Runtime::handle_state_event_envelope(const EnvelopeEvent& ev) {
    if (!matches_local_target(ev.envelope.target) ||
        ev.envelope.message_type != static_cast<uint16_t>(StateEventType::kVehicleEvent)) {
        return;
    }

    VehicleEvent payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        runtime_publish_semantic_decode_error(bus_, ev);
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
    if (!matches_local_target(ev.envelope.target)) {
        return;
    }

    CommandResult payload{};
    if (!decode_typed_payload(ev.envelope.payload, &payload)) {
        runtime_publish_semantic_decode_error(bus_, ev);
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
    if (!matches_local_target(ev.envelope.target) ||
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
        runtime_publish_semantic_decode_error(bus_, ev);
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
