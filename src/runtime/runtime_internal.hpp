/**
 * @file src/runtime/runtime_internal.hpp
 * @brief Runtime 内部共享定义。
 */

#ifndef SUNRAYCOM_RUNTIME_RUNTIME_INTERNAL_HPP
#define SUNRAYCOM_RUNTIME_RUNTIME_INTERNAL_HPP

#include <chrono>
#include <mutex>
#include <unordered_map>

#include "sunraycom/runtime/runtime.hpp"

namespace sunraycom {

struct Runtime::Impl {
    mutable std::mutex mu;
    std::unordered_map<uint64_t, SessionDescriptor> sessions;
    AuthorityLease authority;
    size_t next_token = 1;
    size_t bus_token = 0;
    uint64_t next_session_id = 1;
    uint64_t next_message_id = 1;
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
};

inline uint64_t runtime_now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

inline CommandKind runtime_command_kind_for_message_type(uint16_t message_type) {
    return static_cast<CommandKind>(message_type);
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

}  // namespace sunraycom

#endif  // SUNRAYCOM_RUNTIME_RUNTIME_INTERNAL_HPP
