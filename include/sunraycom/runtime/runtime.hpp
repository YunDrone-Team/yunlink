/**
 * @file include/sunraycom/runtime/runtime.hpp
 * @brief Runtime 聚合组件定义。
 */

#ifndef SUNRAYCOM_RUNTIME_RUNTIME_HPP
#define SUNRAYCOM_RUNTIME_RUNTIME_HPP

#include <functional>
#include <memory>
#include <string>

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/runtime_config.hpp"
#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/transport/tcp_client_pool.hpp"
#include "sunraycom/transport/tcp_server.hpp"
#include "sunraycom/transport/udp_transport.hpp"

namespace sunraycom {

class Runtime;

class SessionClient {
  public:
    explicit SessionClient(Runtime* runtime = nullptr);

    uint64_t open_active_session(const std::string& peer_id, const std::string& node_name);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class SessionServer {
  public:
    explicit SessionServer(Runtime* runtime = nullptr);

    bool has_active_session(uint64_t session_id) const;
    bool describe_session(uint64_t session_id, SessionDescriptor* out) const;
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class CommandPublisher {
  public:
    explicit CommandPublisher(Runtime* runtime = nullptr);

    ErrorCode publish_takeoff(const std::string& peer_id,
                              uint64_t session_id,
                              const TargetSelector& target,
                              const TakeoffCommand& payload,
                              CommandHandle* out_handle = nullptr);
    ErrorCode publish_land(const std::string& peer_id,
                           uint64_t session_id,
                           const TargetSelector& target,
                           const LandCommand& payload,
                           CommandHandle* out_handle = nullptr);
    ErrorCode publish_return(const std::string& peer_id,
                             uint64_t session_id,
                             const TargetSelector& target,
                             const ReturnCommand& payload,
                             CommandHandle* out_handle = nullptr);
    ErrorCode publish_goto(const std::string& peer_id,
                           uint64_t session_id,
                           const TargetSelector& target,
                           const GotoCommand& payload,
                           CommandHandle* out_handle = nullptr);
    ErrorCode publish_velocity_setpoint(const std::string& peer_id,
                                        uint64_t session_id,
                                        const TargetSelector& target,
                                        const VelocitySetpointCommand& payload,
                                        CommandHandle* out_handle = nullptr);
    ErrorCode publish_trajectory_chunk(const std::string& peer_id,
                                       uint64_t session_id,
                                       const TargetSelector& target,
                                       const TrajectoryChunkCommand& payload,
                                       CommandHandle* out_handle = nullptr);
    ErrorCode publish_formation_task(const std::string& peer_id,
                                     uint64_t session_id,
                                     const TargetSelector& target,
                                     const FormationTaskCommand& payload,
                                     CommandHandle* out_handle = nullptr);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class StateSubscriber {
  public:
    using VehicleCoreHandler = std::function<void(const TypedMessage<VehicleCoreState>&)>;
    using Px4StateHandler = std::function<void(const TypedMessage<Px4StateSnapshot>&)>;
    using OdomStatusHandler = std::function<void(const TypedMessage<OdomStatusSnapshot>&)>;
    using UavControlFsmStateHandler =
        std::function<void(const TypedMessage<UavControlFsmStateSnapshot>&)>;
    using UavControllerStateHandler =
        std::function<void(const TypedMessage<UavControllerStateSnapshot>&)>;
    using GimbalParamsHandler = std::function<void(const TypedMessage<GimbalParamsSnapshot>&)>;

    explicit StateSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_vehicle_core(VehicleCoreHandler cb);
    size_t subscribe_px4_state(Px4StateHandler cb);
    size_t subscribe_odom_status(OdomStatusHandler cb);
    size_t subscribe_uav_control_fsm_state(UavControlFsmStateHandler cb);
    size_t subscribe_uav_controller_state(UavControllerStateHandler cb);
    size_t subscribe_gimbal_params(GimbalParamsHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

class EventSubscriber {
  public:
    using VehicleEventHandler = std::function<void(const TypedMessage<VehicleEvent>&)>;
    using CommandResultHandler = std::function<void(const CommandResultView&)>;

    explicit EventSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_vehicle_event(VehicleEventHandler cb);
    size_t subscribe_command_results(CommandResultHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

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
    CommandPublisher& command_publisher() {
        return command_publisher_;
    }
    StateSubscriber& state_subscriber() {
        return state_subscriber_;
    }
    EventSubscriber& event_subscriber() {
        return event_subscriber_;
    }

    ErrorCode request_authority(const std::string& peer_id,
                                uint64_t session_id,
                                const TargetSelector& target,
                                ControlSource source,
                                uint32_t lease_ttl_ms,
                                bool allow_preempt = false);
    ErrorCode release_authority(const std::string& peer_id,
                                uint64_t session_id,
                                const TargetSelector& target);
    bool current_authority(AuthorityLease* out) const;

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
    ErrorCode publish_vehicle_event(const std::string& peer_id,
                                    const TargetSelector& target,
                                    const VehicleEvent& payload,
                                    uint64_t session_id = 0);

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
    StateSubscriber state_subscriber_;
    EventSubscriber event_subscriber_;
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
    ErrorCode send_envelope_to_peer(const std::string& peer_id, const SecureEnvelope& envelope);
    ErrorCode reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope);
    bool describe_session_internal(uint64_t session_id, SessionDescriptor* out) const;
    void unsubscribe_semantic(size_t token);
    size_t subscribe_vehicle_core_internal(StateSubscriber::VehicleCoreHandler cb);
    size_t subscribe_px4_state_internal(StateSubscriber::Px4StateHandler cb);
    size_t subscribe_odom_status_internal(StateSubscriber::OdomStatusHandler cb);
    size_t subscribe_uav_control_fsm_state_internal(StateSubscriber::UavControlFsmStateHandler cb);
    size_t subscribe_uav_controller_state_internal(StateSubscriber::UavControllerStateHandler cb);
    size_t subscribe_gimbal_params_internal(StateSubscriber::GimbalParamsHandler cb);
    size_t subscribe_vehicle_event_internal(EventSubscriber::VehicleEventHandler cb);
    size_t subscribe_command_result_internal(EventSubscriber::CommandResultHandler cb);
    void handle_session_envelope(const EnvelopeEvent& ev);
    void handle_authority_envelope(const EnvelopeEvent& ev);
    void handle_command_envelope(const EnvelopeEvent& ev);
    void handle_state_snapshot_envelope(const EnvelopeEvent& ev);
    void handle_state_event_envelope(const EnvelopeEvent& ev);
    void handle_command_result_envelope(const EnvelopeEvent& ev);
    void handle_envelope(const EnvelopeEvent& ev);
    void publish_command_result_sequence(const EnvelopeEvent& inbound, const SecureEnvelope& cmd);

    friend class SessionClient;
    friend class SessionServer;
    friend class CommandPublisher;
    friend class StateSubscriber;
    friend class EventSubscriber;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_RUNTIME_RUNTIME_HPP
