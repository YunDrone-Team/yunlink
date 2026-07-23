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

yunlink_result_t yunlink_command_publish_uav_control(yunlink_runtime_t* runtime,
                                                     const yunlink_peer_t* peer,
                                                     const yunlink_session_t* session,
                                                     const yunlink_target_selector_t* target,
                                                     const yunlink_uav_control_command_t* payload,
                                                     yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::UavControlCommand native{};
    native.control_cmd = payload->control_cmd;
    native.desired_position_m = {payload->desired_position_x_m,
                                 payload->desired_position_y_m,
                                 payload->desired_position_z_m};
    native.desired_velocity_mps = {payload->desired_velocity_x_mps,
                                   payload->desired_velocity_y_mps,
                                   payload->desired_velocity_z_mps};
    native.desired_acceleration_mps2 = {payload->desired_acceleration_x_mps2,
                                        payload->desired_acceleration_y_mps2,
                                        payload->desired_acceleration_z_mps2};
    native.desired_body_xy_position_m = {payload->desired_body_xy_position_x_m,
                                         payload->desired_body_xy_position_y_m};
    native.desired_body_xy_velocity_mps = {payload->desired_body_xy_velocity_x_mps,
                                           payload->desired_body_xy_velocity_y_mps};
    native.fixed_height_m = payload->fixed_height_m;
    native.yaw_mode = payload->yaw_mode;
    native.desired_yaw_rad = payload->desired_yaw_rad;
    native.desired_yaw_rate_radps = payload->desired_yaw_rate_radps;
    native.controller_type = payload->controller_type;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_uav_control(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_ugv_control(yunlink_runtime_t* runtime,
                                                     const yunlink_peer_t* peer,
                                                     const yunlink_session_t* session,
                                                     const yunlink_target_selector_t* target,
                                                     const yunlink_ugv_control_command_t* payload,
                                                     yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::UgvControlCommand native{};
    native.control_cmd = payload->control_cmd;
    native.desired_position_m = {payload->desired_position_x_m,
                                 payload->desired_position_y_m,
                                 payload->desired_position_z_m};
    native.desired_velocity_mps = {payload->desired_velocity_x_mps,
                                   payload->desired_velocity_y_mps,
                                   payload->desired_velocity_z_mps};
    native.body_linear_velocity_mps = {payload->body_linear_velocity_x_mps,
                                       payload->body_linear_velocity_y_mps,
                                       payload->body_linear_velocity_z_mps};
    native.body_angular_velocity_radps = {payload->body_angular_velocity_x_radps,
                                          payload->body_angular_velocity_y_radps,
                                          payload->body_angular_velocity_z_radps};
    native.desired_yaw_rad = payload->desired_yaw_rad;
    native.desired_wgs84_latitude_deg = payload->desired_wgs84_latitude_deg;
    native.desired_wgs84_longitude_deg = payload->desired_wgs84_longitude_deg;
    native.desired_wgs84_altitude_m = payload->desired_wgs84_altitude_m;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_ugv_control(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

}  // extern "C"
