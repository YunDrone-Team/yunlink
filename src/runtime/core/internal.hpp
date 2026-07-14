/**
 * @file src/runtime/runtime_internal.hpp
 * @brief Runtime 内部共享定义。
 */

#ifndef YUNLINK_RUNTIME_RUNTIME_INTERNAL_HPP
#define YUNLINK_RUNTIME_RUNTIME_INTERNAL_HPP

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <mutex>
#include <string>
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
    std::unordered_map<size_t, CommandSubscriber::TrajectoryChunkHandler> trajectory_chunk_handlers;
    std::unordered_map<size_t, CommandSubscriber::FormationTaskHandler> formation_task_handlers;
    std::unordered_map<size_t, StateSubscriber::VehicleCoreHandler> vehicle_core_handlers;
    std::unordered_map<size_t, StateSubscriber::Px4StateHandler> px4_state_handlers;
    std::unordered_map<size_t, StateSubscriber::OdomStatusHandler> odom_status_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControlFsmStateHandler>
        uav_control_fsm_state_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControllerStateHandler>
        uav_controller_state_handlers;
    std::unordered_map<size_t, StateSubscriber::GimbalParamsHandler> gimbal_params_handlers;
    std::unordered_map<size_t, StateSubscriber::LocalOdomHandler> local_odom_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControlCmdHandler> uav_control_cmd_handlers;
    std::unordered_map<size_t, StateSubscriber::UavControlStateHandler> uav_control_state_handlers;
    std::unordered_map<size_t, StateSubscriber::CommandExecutionStatusHandler>
        command_execution_status_handlers;
    std::unordered_map<size_t, StateSubscriber::OdomStateHandler> odom_state_handlers;
    std::unordered_map<size_t, StateSubscriber::SunrayRuntimeDiagnosticHandler>
        sunray_runtime_diagnostic_handlers;
    std::unordered_map<size_t, EventSubscriber::VehicleEventHandler> vehicle_event_handlers;
    std::unordered_map<size_t, EventSubscriber::CommandResultHandler> command_result_handlers;
    std::unordered_map<size_t, EventSubscriber::AuthorityStatusHandler> authority_status_handlers;
    std::unordered_map<size_t, size_t> packet_trace_bus_tokens;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureListRequestHandler>
        feature_list_request_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureListResponseHandler>
        feature_list_response_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureGetRequestHandler>
        feature_get_request_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureGetResponseHandler>
        feature_get_response_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureStartRequestHandler>
        feature_start_request_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureStartResponseHandler>
        feature_start_response_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureStopRequestHandler>
        feature_stop_request_handlers;
    std::unordered_map<size_t, SystemServiceSubscriber::FeatureStopResponseHandler>
        feature_stop_response_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceListRequestHandler>
        config_resource_list_request_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceListResponseHandler>
        config_resource_list_response_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceDescribeRequestHandler>
        config_resource_describe_request_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceDescribeResponseHandler>
        config_resource_describe_response_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceGetRequestHandler>
        config_resource_get_request_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceGetResponseHandler>
        config_resource_get_response_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourcePatchRequestHandler>
        config_resource_patch_request_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourcePatchResponseHandler>
        config_resource_patch_response_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceApplyRequestHandler>
        config_resource_apply_request_handlers;
    std::unordered_map<size_t, ConfigurationServiceSubscriber::ResourceApplyResponseHandler>
        config_resource_apply_response_handlers;
    std::unordered_map<size_t, EventSubscriber::BulkChannelDescriptorHandler>
        bulk_channel_descriptor_handlers;
    std::unordered_map<uint32_t, BulkChannelDescriptor> active_bulk_channels;
    std::unordered_map<std::string, uint64_t> reliable_latest_watermarks;
    std::unordered_map<std::string, uint64_t> command_result_from_status_seen;
    std::unordered_map<std::string, RuntimeTrajectoryAccumulator> trajectory_accumulators;
    std::unordered_set<std::string> security_replay_keys;
};

inline PacketTraceStoreConfig runtime_packet_trace_config(const RuntimeConfig& config) {
    PacketTraceStoreConfig out;
    out.enabled = config.packet_trace_enabled;
    out.max_records = config.packet_trace_max_records;
    out.max_total_bytes = config.packet_trace_max_total_bytes;
    out.raw_preview_bytes = config.packet_trace_raw_preview_bytes;
    out.payload_preview_bytes = config.packet_trace_payload_preview_bytes;
    return out;
}

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
           std::to_string(envelope.message_type) + ":" + std::to_string(envelope.session_id) + ":" +
           std::to_string(static_cast<uint8_t>(envelope.source.agent_type)) + ":" +
           std::to_string(envelope.source.agent_id) + ":" + runtime_target_key(envelope.target);
}

inline void runtime_publish_packet_trace(EventBus& bus,
                                         const RuntimeConfig& config,
                                         PacketTraceDirection direction,
                                         PacketTraceStage stage,
                                         TransportType transport,
                                         const PeerInfo& peer,
                                         const SecureEnvelope& envelope,
                                         const uint8_t* raw_data = nullptr,
                                         size_t raw_len = 0,
                                         ErrorCode code = ErrorCode::kOk,
                                         const std::string& detail = std::string()) {
    if (!config.packet_trace_enabled) {
        return;
    }
    ByteBuffer encoded;
    if (raw_data == nullptr && direction == PacketTraceDirection::kTx) {
        encoded = ProtocolCodec().encode(envelope);
        raw_data = encoded.data();
        raw_len = encoded.size();
    }
    bus.publish_packet_trace(make_packet_trace_record(direction,
                                                      stage,
                                                      transport,
                                                      peer,
                                                      &envelope,
                                                      raw_data,
                                                      raw_len,
                                                      code,
                                                      detail,
                                                      config.packet_trace_raw_preview_bytes,
                                                      config.packet_trace_payload_preview_bytes));
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
    envelope.schema_version = kCurrentSchemaVersion;
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
