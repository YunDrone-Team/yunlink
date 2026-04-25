/**
 * @file src/runtime/runtime_internal.hpp
 * @brief Runtime 内部共享定义。
 */

#ifndef YUNLINK_RUNTIME_RUNTIME_INTERNAL_HPP
#define YUNLINK_RUNTIME_RUNTIME_INTERNAL_HPP

#include <algorithm>
#include <chrono>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

#include "yunlink/runtime/runtime.hpp"

namespace yunlink {

struct RuntimeTrajectoryAccumulator {
    uint32_t next_chunk_index = 0;
    uint64_t updated_at_ms = 0;
    TrajectoryChunkCommand assembled;
};

struct Runtime::Impl {
    mutable std::mutex mu;
    std::unordered_map<std::string, SessionDescriptor> sessions;
    std::unordered_map<std::string, AuthorityLease> authorities;
    size_t next_token = 1;
    size_t bus_token = 0;
    size_t link_bus_token = 0;
    uint64_t next_session_id = 1;
    uint64_t next_message_id = 1;
    std::unordered_map<size_t, CommandSubscriber::TakeoffHandler> takeoff_handlers;
    std::unordered_map<size_t, CommandSubscriber::LandHandler> land_handlers;
    std::unordered_map<size_t, CommandSubscriber::ReturnHandler> return_handlers;
    std::unordered_map<size_t, CommandSubscriber::GotoHandler> goto_handlers;
    std::unordered_map<size_t, CommandSubscriber::VelocitySetpointHandler>
        velocity_setpoint_handlers;
    std::unordered_map<size_t, CommandSubscriber::TrajectoryChunkHandler>
        trajectory_chunk_handlers;
    std::unordered_map<size_t, CommandSubscriber::FormationTaskHandler> formation_task_handlers;
    std::unordered_map<size_t, StateSubscriber::VehicleCoreHandler> vehicle_core_handlers;
    std::unordered_map<size_t, StateSubscriber::Px4StateHandler> px4_state_handlers;
    std::unordered_map<size_t, StateSubscriber::OdomStatusHandler> odom_status_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControlFsmStateHandler>
        uav_control_fsm_state_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControllerStateHandler>
        uav_controller_state_handlers;
    std::unordered_map<size_t, StateSubscriber::GimbalParamsHandler> gimbal_params_handlers;
    std::unordered_map<size_t, EventSubscriber::VehicleEventHandler> vehicle_event_handlers;
    std::unordered_map<size_t, EventSubscriber::CommandResultHandler> command_result_handlers;
    std::unordered_map<size_t, EventSubscriber::AuthorityStatusHandler> authority_status_handlers;
    std::unordered_map<size_t, EventSubscriber::BulkChannelDescriptorHandler>
        bulk_channel_descriptor_handlers;
    std::unordered_map<uint32_t, BulkChannelDescriptor> active_bulk_channels;
    std::unordered_map<std::string, uint64_t> reliable_latest_watermarks;
    std::unordered_map<std::string, RuntimeTrajectoryAccumulator> trajectory_accumulators;
    std::unordered_set<std::string> security_replay_keys;
};

inline uint64_t runtime_now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

inline CommandKind runtime_command_kind_for_message_type(uint16_t message_type) {
    return static_cast<CommandKind>(message_type);
}

inline std::string runtime_session_key(const std::string& peer_id, uint64_t session_id) {
    return peer_id + "#" + std::to_string(session_id);
}

inline std::string runtime_target_key(const TargetSelector& target) {
    std::vector<uint32_t> ids = target.target_ids;
    std::sort(ids.begin(), ids.end());

    std::string key = std::to_string(static_cast<uint8_t>(target.scope)) + ":" +
                      std::to_string(static_cast<uint8_t>(target.target_type)) + ":" +
                      std::to_string(target.group_id) + ":";
    for (uint32_t id : ids) {
        key += std::to_string(id);
        key += ",";
    }
    return key;
}

inline std::string runtime_trajectory_key(const EnvelopeEvent& ev) {
    return runtime_session_key(ev.peer.id, ev.envelope.session_id) + "#" +
           runtime_target_key(ev.envelope.target);
}

inline std::string runtime_qos_latest_key(const SecureEnvelope& envelope) {
    return std::to_string(static_cast<uint8_t>(envelope.message_family)) + ":" +
           std::to_string(envelope.message_type) + ":" +
           std::to_string(envelope.session_id) + ":" +
           std::to_string(static_cast<uint8_t>(envelope.source.agent_type)) + ":" +
           std::to_string(envelope.source.agent_id) + ":" + runtime_target_key(envelope.target);
}

inline SecureEnvelope make_runtime_envelope(const EndpointIdentity& source,
                                            const TargetSelector& target,
                                            uint64_t session_id,
                                            uint64_t correlation_id,
                                            QosClass qos_class,
                                            MessageFamily message_family,
                                            uint16_t message_type,
                                            const ByteBuffer& payload,
                                            uint32_t ttl_ms) {
    SecureEnvelope envelope;
    envelope.qos_class = qos_class;
    envelope.message_family = message_family;
    envelope.message_type = message_type;
    envelope.schema_version = 1;
    envelope.session_id = session_id;
    envelope.correlation_id = correlation_id;
    envelope.source = source;
    envelope.target = target;
    envelope.ttl_ms = ttl_ms;
    envelope.payload = payload;
    envelope.payload_len = static_cast<uint32_t>(payload.size());
    envelope.created_at_ms = runtime_now_millis();
    envelope.message_id = envelope.created_at_ms;
    envelope.header_len = static_cast<uint16_t>(
        ProtocolCodec::kFixedHeaderSize + envelope.target.target_ids.size() * sizeof(uint32_t));
    return envelope;
}

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_RUNTIME_INTERNAL_HPP
