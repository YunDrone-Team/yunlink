/**
 * @file include/yunlink/runtime/runtime_class.hpp
 * @brief Runtime aggregate component definition.
 */

#ifndef YUNLINK_RUNTIME_RUNTIME_CLASS_HPP
#define YUNLINK_RUNTIME_RUNTIME_CLASS_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/runtime_config.hpp"
#include "yunlink/runtime/command.hpp"
#include "yunlink/runtime/configuration_service.hpp"
#include "yunlink/runtime/events.hpp"
#include "yunlink/runtime/session.hpp"
#include "yunlink/runtime/state.hpp"
#include "yunlink/runtime/system_service.hpp"
#include "yunlink/transport/tcp_client_pool.hpp"
#include "yunlink/transport/tcp_server.hpp"
#include "yunlink/transport/udp_transport.hpp"

namespace yunlink {

class Runtime {
  public:
    Runtime();
    ~Runtime();

    ErrorCode start(const RuntimeConfig& config);
    void stop();

    EventBus& event_bus() {
        return bus_;
    }
    UdpTransport& udp() {
        return udp_;
    }
    TcpClientPool& tcp_clients() {
        return tcp_clients_;
    }
    TcpServer& tcp_server() {
        return tcp_server_;
    }

    SessionClient& session_client() {
        return session_client_;
    }
    SessionServer& session_server() {
        return session_server_;
    }
    const SessionServer& session_server() const {
        return session_server_;
    }
    CommandPublisher& command_publisher() {
        return command_publisher_;
    }
    CommandSubscriber& command_subscriber() {
        return command_subscriber_;
    }
    StateSubscriber& state_subscriber() {
        return state_subscriber_;
    }
    EventSubscriber& event_subscriber() {
        return event_subscriber_;
    }
    SystemServicePublisher& system_service_publisher() {
        return system_service_publisher_;
    }
    SystemServiceSubscriber& system_service_subscriber() {
        return system_service_subscriber_;
    }
    ConfigurationServicePublisher& configuration_service_publisher() {
        return configuration_service_publisher_;
    }
    ConfigurationServiceSubscriber& configuration_service_subscriber() {
        return configuration_service_subscriber_;
    }

    ErrorCode request_authority(const std::string& peer_id,
                                uint64_t session_id,
                                const TargetSelector& target,
                                ControlSource source,
                                uint32_t lease_ttl_ms,
                                bool allow_preempt = false);
    ErrorCode renew_authority(const std::string& peer_id,
                              uint64_t session_id,
                              const TargetSelector& target,
                              ControlSource source,
                              uint32_t lease_ttl_ms);
    ErrorCode release_authority(const std::string& peer_id,
                                uint64_t session_id,
                                const TargetSelector& target);
    bool current_authority(AuthorityLease* out) const;
    bool current_authority_for_target(const TargetSelector& target, AuthorityLease* out) const;

    ErrorCode publish_vehicle_core_state(const std::string& peer_id,
                                         const TargetSelector& target,
                                         const VehicleCoreState& payload,
                                         uint64_t session_id = 0);
    ErrorCode publish_px4_state(const std::string& peer_id,
                                const TargetSelector& target,
                                const Px4StateSnapshot& payload,
                                uint64_t session_id = 0);
    ErrorCode publish_odom_status(const std::string& peer_id,
                                  const TargetSelector& target,
                                  const OdomStatusSnapshot& payload,
                                  uint64_t session_id = 0);
    ErrorCode publish_uav_control_fsm_state(const std::string& peer_id,
                                            const TargetSelector& target,
                                            const UavControlFsmStateSnapshot& payload,
                                            uint64_t session_id = 0);
    ErrorCode publish_uav_controller_state(const std::string& peer_id,
                                           const TargetSelector& target,
                                           const UavControllerStateSnapshot& payload,
                                           uint64_t session_id = 0);
    ErrorCode publish_gimbal_params(const std::string& peer_id,
                                    const TargetSelector& target,
                                    const GimbalParamsSnapshot& payload,
                                    uint64_t session_id = 0);
    ErrorCode publish_local_odom(const std::string& peer_id,
                                 const TargetSelector& target,
                                 const LocalOdomSnapshot& payload,
                                 uint64_t session_id = 0);
    ErrorCode publish_uav_control_cmd(const std::string& peer_id,
                                      const TargetSelector& target,
                                      const UavControlCmdSnapshot& payload,
                                      uint64_t session_id = 0);
    ErrorCode publish_uav_control_state(const std::string& peer_id,
                                        const TargetSelector& target,
                                        const UavControlStateSnapshot& payload,
                                        uint64_t session_id = 0);
    ErrorCode publish_command_execution_status(const std::string& peer_id,
                                               const TargetSelector& target,
                                               const CommandExecutionStatusSnapshot& payload,
                                               uint64_t session_id = 0);
    ErrorCode publish_odom_state(const std::string& peer_id,
                                 const TargetSelector& target,
                                 const OdomStateSnapshot& payload,
                                 uint64_t session_id = 0);
    ErrorCode publish_sunray_runtime_diagnostic(const std::string& peer_id,
                                                const TargetSelector& target,
                                                const SunrayRuntimeDiagnosticSnapshot& payload,
                                                uint64_t session_id = 0);
    ErrorCode publish_vehicle_event(const std::string& peer_id,
                                    const TargetSelector& target,
                                    const VehicleEvent& payload,
                                    uint64_t session_id = 0);
    ErrorCode publish_bulk_channel_descriptor(const std::string& peer_id,
                                              const TargetSelector& target,
                                              const BulkChannelDescriptor& payload,
                                              uint64_t session_id = 0);
    ErrorCode reply_command_result(const EnvelopeEvent& inbound,
                                   const CommandResult& payload,
                                   uint32_t ttl_ms = 1000);
    bool current_bulk_channel(uint32_t channel_id, BulkChannelDescriptor* out) const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    RuntimeConfig config_;
    EventBus bus_;
    UdpTransport udp_;
    TcpClientPool tcp_clients_;
    TcpServer tcp_server_;
    SessionClient session_client_;
    SessionServer session_server_;
    CommandPublisher command_publisher_;
    CommandSubscriber command_subscriber_;
    StateSubscriber state_subscriber_;
    EventSubscriber event_subscriber_;
    SystemServicePublisher system_service_publisher_;
    SystemServiceSubscriber system_service_subscriber_;
    ConfigurationServicePublisher configuration_service_publisher_;
    ConfigurationServiceSubscriber configuration_service_subscriber_;
    bool is_started_ = false;

    uint64_t allocate_session_id();
    uint64_t allocate_message_id();
    ErrorCode send_session_payload(const std::string& peer_id,
                                   uint64_t session_id,
                                   uint64_t correlation_id,
                                   uint16_t message_type,
                                   const ByteBuffer& payload,
                                   uint32_t ttl_ms);
    ErrorCode publish_command_payload(const std::string& peer_id,
                                      uint64_t session_id,
                                      const TargetSelector& target,
                                      uint16_t message_type,
                                      const ByteBuffer& payload,
                                      CommandHandle* out_handle,
                                      uint32_t ttl_ms);
    ErrorCode publish_system_service_request_payload(const std::string& peer_id,
                                                     uint64_t session_id,
                                                     const TargetSelector& target,
                                                     uint16_t message_type,
                                                     const ByteBuffer& payload,
                                                     SystemServiceHandle* out_handle,
                                                     uint32_t ttl_ms);
    ErrorCode reply_system_service_payload(const EnvelopeEvent& inbound,
                                           uint16_t message_type,
                                           const ByteBuffer& payload,
                                           uint32_t ttl_ms);
#include "yunlink/runtime/configuration/runtime_declarations.inc"
    ErrorCode send_envelope_to_peer(const std::string& peer_id, const SecureEnvelope& envelope);
    ErrorCode reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope);
    bool describe_session_internal(uint64_t session_id, SessionDescriptor* out) const;
    bool describe_session_internal(const std::string& peer_id,
                                   uint64_t session_id,
                                   SessionDescriptor* out) const;
    bool find_active_session_internal(SessionDescriptor* out) const;
    void unsubscribe_semantic(size_t token);
    size_t subscribe_takeoff_internal(CommandSubscriber::TakeoffHandler cb);
    size_t subscribe_land_internal(CommandSubscriber::LandHandler cb);
    size_t subscribe_return_internal(CommandSubscriber::ReturnHandler cb);
    size_t subscribe_goto_internal(CommandSubscriber::GotoHandler cb);
    size_t subscribe_velocity_setpoint_internal(CommandSubscriber::VelocitySetpointHandler cb);
    size_t subscribe_trajectory_chunk_internal(CommandSubscriber::TrajectoryChunkHandler cb);
    size_t subscribe_formation_task_internal(CommandSubscriber::FormationTaskHandler cb);
    size_t subscribe_vehicle_core_internal(StateSubscriber::VehicleCoreHandler cb);
    size_t subscribe_px4_state_internal(StateSubscriber::Px4StateHandler cb);
    size_t subscribe_odom_status_internal(StateSubscriber::OdomStatusHandler cb);
    size_t subscribe_uav_control_fsm_state_internal(StateSubscriber::UavControlFsmStateHandler cb);
    size_t subscribe_uav_controller_state_internal(StateSubscriber::UavControllerStateHandler cb);
    size_t subscribe_gimbal_params_internal(StateSubscriber::GimbalParamsHandler cb);
    size_t subscribe_vehicle_event_internal(EventSubscriber::VehicleEventHandler cb);
    size_t subscribe_command_result_internal(EventSubscriber::CommandResultHandler cb);
    size_t subscribe_authority_status_internal(EventSubscriber::AuthorityStatusHandler cb);
    size_t subscribe_packet_trace_internal(EventSubscriber::PacketTraceHandler cb);
    size_t subscribe_local_odom_internal(StateSubscriber::LocalOdomHandler cb);
    size_t subscribe_uav_control_cmd_internal(StateSubscriber::UavControlCmdHandler cb);
    size_t subscribe_uav_control_state_internal(StateSubscriber::UavControlStateHandler cb);
    size_t
    subscribe_command_execution_status_internal(StateSubscriber::CommandExecutionStatusHandler cb);
    size_t subscribe_odom_state_internal(StateSubscriber::OdomStateHandler cb);
    size_t subscribe_sunray_runtime_diagnostic_internal(
        StateSubscriber::SunrayRuntimeDiagnosticHandler cb);
    size_t
    subscribe_feature_list_request_internal(SystemServiceSubscriber::FeatureListRequestHandler cb);
    size_t subscribe_feature_list_response_internal(
        SystemServiceSubscriber::FeatureListResponseHandler cb);
    size_t
    subscribe_feature_get_request_internal(SystemServiceSubscriber::FeatureGetRequestHandler cb);
    size_t
    subscribe_feature_get_response_internal(SystemServiceSubscriber::FeatureGetResponseHandler cb);
    size_t subscribe_feature_start_request_internal(
        SystemServiceSubscriber::FeatureStartRequestHandler cb);
    size_t subscribe_feature_start_response_internal(
        SystemServiceSubscriber::FeatureStartResponseHandler cb);
    size_t
    subscribe_feature_stop_request_internal(SystemServiceSubscriber::FeatureStopRequestHandler cb);
    size_t subscribe_feature_stop_response_internal(
        SystemServiceSubscriber::FeatureStopResponseHandler cb);
    size_t
    subscribe_bulk_channel_descriptor_internal(EventSubscriber::BulkChannelDescriptorHandler cb);
    void handle_session_envelope(const EnvelopeEvent& ev);
    void handle_authority_envelope(const EnvelopeEvent& ev);
    void handle_command_envelope(const EnvelopeEvent& ev);
    void handle_state_snapshot_envelope(const EnvelopeEvent& ev);
    void handle_command_execution_status_snapshot(const EnvelopeEvent& ev);
    void handle_state_event_envelope(const EnvelopeEvent& ev);
    void handle_command_result_envelope(const EnvelopeEvent& ev);
    void handle_bulk_channel_descriptor_envelope(const EnvelopeEvent& ev);
    void handle_system_service_envelope(const EnvelopeEvent& ev);
    void handle_configuration_service_envelope(const EnvelopeEvent& ev);
    void handle_envelope(const EnvelopeEvent& ev);
    void handle_link_event(const LinkEvent& ev);
    void publish_command_result_sequence(const EnvelopeEvent& inbound, const SecureEnvelope& cmd);

    friend class SessionClient;
    friend class SessionServer;
    friend class CommandPublisher;
    friend class CommandSubscriber;
    friend class StateSubscriber;
    friend class EventSubscriber;
    friend class SystemServicePublisher;
    friend class SystemServiceSubscriber;
    friend class ConfigurationServicePublisher;
    friend class ConfigurationServiceSubscriber;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_RUNTIME_CLASS_HPP
