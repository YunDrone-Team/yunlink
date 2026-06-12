/**
 * @file include/yunlink/runtime/state.hpp
 * @brief Runtime state subscription facade declarations.
 */

#ifndef YUNLINK_RUNTIME_STATE_HPP
#define YUNLINK_RUNTIME_STATE_HPP

#include <cstddef>
#include <functional>

#include "yunlink/core/semantic_messages.hpp"

namespace yunlink {

class Runtime;

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
    using LocalOdomHandler = std::function<void(const TypedMessage<LocalOdomSnapshot>&)>;
    using UavControlCmdHandler = std::function<void(const TypedMessage<UavControlCmdSnapshot>&)>;
    using UavControlStateHandler =
        std::function<void(const TypedMessage<UavControlStateSnapshot>&)>;
    using OdomStateHandler = std::function<void(const TypedMessage<OdomStateSnapshot>&)>;
    using SunrayRuntimeDiagnosticHandler =
        std::function<void(const TypedMessage<SunrayRuntimeDiagnosticSnapshot>&)>;
    using CommandExecutionStatusHandler =
        std::function<void(const TypedMessage<CommandExecutionStatusSnapshot>&)>;

    explicit StateSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_vehicle_core(VehicleCoreHandler cb);
    size_t subscribe_px4_state(Px4StateHandler cb);
    size_t subscribe_odom_status(OdomStatusHandler cb);
    size_t subscribe_uav_control_fsm_state(UavControlFsmStateHandler cb);
    size_t subscribe_uav_controller_state(UavControllerStateHandler cb);
    size_t subscribe_gimbal_params(GimbalParamsHandler cb);
    size_t subscribe_local_odom(LocalOdomHandler cb);
    size_t subscribe_uav_control_cmd(UavControlCmdHandler cb);
    size_t subscribe_uav_control_state(UavControlStateHandler cb);
    size_t subscribe_odom_state(OdomStateHandler cb);
    size_t subscribe_sunray_runtime_diagnostic(SunrayRuntimeDiagnosticHandler cb);
    size_t subscribe_command_execution_status(CommandExecutionStatusHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_STATE_HPP
