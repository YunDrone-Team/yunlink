/**
 * @file src/c/abi/command.cpp
 * @brief C ABI command publish functions.
 */

#include "../internal.hpp"

using namespace yunlink_c_abi;

extern "C" {

yunlink_result_t yunlink_command_publish_takeoff(yunlink_runtime_t* runtime,
                                                 const yunlink_peer_t* peer,
                                                 const yunlink_session_t* session,
                                                 const yunlink_target_selector_t* target,
                                                 const yunlink_takeoff_command_t* payload,
                                                 yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::TakeoffCommand native{};
    native.reserved = payload->reserved;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_takeoff(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_land(yunlink_runtime_t* runtime,
                                              const yunlink_peer_t* peer,
                                              const yunlink_session_t* session,
                                              const yunlink_target_selector_t* target,
                                              const yunlink_land_command_t* payload,
                                              yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::LandCommand native{};
    native.reserved = payload->reserved;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_land(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_return(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                const yunlink_return_command_t* payload,
                                                yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::ReturnCommand native{};
    native.reserved = payload->reserved;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_return(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_goto(yunlink_runtime_t* runtime,
                                              const yunlink_peer_t* peer,
                                              const yunlink_session_t* session,
                                              const yunlink_target_selector_t* target,
                                              const yunlink_goto_command_t* payload,
                                              yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::GotoCommand native{};
    native.x_m = payload->x_m;
    native.y_m = payload->y_m;
    native.z_m = payload->z_m;
    native.yaw_rad = payload->yaw_rad;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_goto(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t
yunlink_command_publish_velocity_setpoint(yunlink_runtime_t* runtime,
                                          const yunlink_peer_t* peer,
                                          const yunlink_session_t* session,
                                          const yunlink_target_selector_t* target,
                                          const yunlink_velocity_setpoint_command_t* payload,
                                          yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::VelocitySetpointCommand native{};
    native.vx_mps = payload->vx_mps;
    native.vy_mps = payload->vy_mps;
    native.vz_mps = payload->vz_mps;
    native.yaw_rate_radps = payload->yaw_rate_radps;
    native.body_frame = payload->body_frame != 0;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_velocity_setpoint(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

}  // extern "C"
