/**
 * @file src/runtime/runtime_state_publish.cpp
 * @brief Runtime 状态发布实现。
 */

#include "runtime_internal.hpp"

namespace yunlink {

ErrorCode Runtime::publish_vehicle_core_state(const std::string& peer_id,
                                              const TargetSelector& target,
                                              const VehicleCoreState& payload,
                                              uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_px4_state(const std::string& peer_id,
                                     const TargetSelector& target,
                                     const Px4StateSnapshot& payload,
                                     uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_odom_status(const std::string& peer_id,
                                       const TargetSelector& target,
                                       const OdomStatusSnapshot& payload,
                                       uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_uav_control_fsm_state(const std::string& peer_id,
                                                 const TargetSelector& target,
                                                 const UavControlFsmStateSnapshot& payload,
                                                 uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_uav_controller_state(const std::string& peer_id,
                                                const TargetSelector& target,
                                                const UavControllerStateSnapshot& payload,
                                                uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableLatest, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

ErrorCode Runtime::publish_gimbal_params(const std::string& peer_id,
                                         const TargetSelector& target,
                                         const GimbalParamsSnapshot& payload,
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

ErrorCode Runtime::publish_bulk_channel_descriptor(const std::string& peer_id,
                                                   const TargetSelector& target,
                                                   const BulkChannelDescriptor& payload,
                                                   uint64_t session_id) {
    SecureEnvelope envelope = make_typed_envelope(
        config_.self_identity, target, session_id, 0, QosClass::kReliableOrdered, payload);
    envelope.message_id = allocate_message_id();
    return send_envelope_to_peer(peer_id, envelope);
}

}  // namespace yunlink
