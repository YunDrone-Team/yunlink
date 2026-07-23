/**
 * @file include/yunlink/runtime/command.hpp
 * @brief Runtime command publish and subscription facades.
 */

#ifndef YUNLINK_RUNTIME_COMMAND_HPP
#define YUNLINK_RUNTIME_COMMAND_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace yunlink {

class Runtime;

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
    ErrorCode publish_uav_control(const std::string& peer_id,
                                  uint64_t session_id,
                                  const TargetSelector& target,
                                  const UavControlCommand& payload,
                                  CommandHandle* out_handle = nullptr);
    ErrorCode publish_ugv_control(const std::string& peer_id,
                                  uint64_t session_id,
                                  const TargetSelector& target,
                                  const UgvControlCommand& payload,
                                  CommandHandle* out_handle = nullptr);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

template <typename T> struct InboundCommandView {
    EnvelopeEvent inbound;
    T payload;
};

class CommandSubscriber {
  public:
    using TakeoffHandler = std::function<void(const InboundCommandView<TakeoffCommand>&)>;
    using LandHandler = std::function<void(const InboundCommandView<LandCommand>&)>;
    using ReturnHandler = std::function<void(const InboundCommandView<ReturnCommand>&)>;
    using GotoHandler = std::function<void(const InboundCommandView<GotoCommand>&)>;
    using VelocitySetpointHandler =
        std::function<void(const InboundCommandView<VelocitySetpointCommand>&)>;
    using TrajectoryChunkHandler =
        std::function<void(const InboundCommandView<TrajectoryChunkCommand>&)>;
    using FormationTaskHandler =
        std::function<void(const InboundCommandView<FormationTaskCommand>&)>;
    using UavControlHandler = std::function<void(const InboundCommandView<UavControlCommand>&)>;
    using UgvControlHandler = std::function<void(const InboundCommandView<UgvControlCommand>&)>;

    explicit CommandSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_takeoff(TakeoffHandler cb);
    size_t subscribe_land(LandHandler cb);
    size_t subscribe_return(ReturnHandler cb);
    size_t subscribe_goto(GotoHandler cb);
    size_t subscribe_velocity_setpoint(VelocitySetpointHandler cb);
    size_t subscribe_trajectory_chunk(TrajectoryChunkHandler cb);
    size_t subscribe_formation_task(FormationTaskHandler cb);
    size_t subscribe_uav_control(UavControlHandler cb);
    size_t subscribe_ugv_control(UgvControlHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_COMMAND_HPP
