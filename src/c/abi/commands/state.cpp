/**
 * @file src/c/abi/state.cpp
 * @brief C ABI state publish functions.
 */

#include "../internal.hpp"

#include <algorithm>

using namespace yunlink_c_abi;

extern "C" {

yunlink_result_t yunlink_publish_vehicle_core_state(yunlink_runtime_t* runtime,
                                                    const yunlink_peer_t* peer,
                                                    const yunlink_target_selector_t* target,
                                                    const yunlink_vehicle_core_state_t* payload,
                                                    uint64_t session_id) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_target(target) ||
        payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::VehicleCoreState native{};
    native.armed = payload->armed != 0;
    native.nav_mode = payload->nav_mode;
    native.x_m = payload->x_m;
    native.y_m = payload->y_m;
    native.z_m = payload->z_m;
    native.vx_mps = payload->vx_mps;
    native.vy_mps = payload->vy_mps;
    native.vz_mps = payload->vz_mps;
    native.battery_percent = payload->battery_percent;
    return to_result(runtime->runtime.publish_vehicle_core_state(
        peer->id, to_target_selector(*target), native, session_id));
}

yunlink_result_t yunlink_publish_local_odom(yunlink_runtime_t* runtime,
                                            const yunlink_peer_t* peer,
                                            const yunlink_target_selector_t* target,
                                            const yunlink_local_odom_t* payload,
                                            uint64_t session_id) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_target(target) ||
        payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::LocalOdomSnapshot native{};
    native.header.stamp_ns = payload->source_stamp_ns;
    native.header.frame_id.assign(
        payload->frame_id,
        std::find(payload->frame_id, payload->frame_id + sizeof(payload->frame_id), '\0'));
    native.child_frame_id.assign(
        payload->child_frame_id,
        std::find(payload->child_frame_id,
                  payload->child_frame_id + sizeof(payload->child_frame_id),
                  '\0'));
    native.pose.position_m = {payload->x_m, payload->y_m, payload->z_m};
    native.pose.orientation = {payload->orientation_x,
                               payload->orientation_y,
                               payload->orientation_z,
                               payload->orientation_w};
    native.twist.linear_mps = {payload->vx_mps, payload->vy_mps, payload->vz_mps};
    native.twist.angular_radps = {
        payload->angular_x_radps, payload->angular_y_radps, payload->angular_z_radps};
    return to_result(runtime->runtime.publish_local_odom(
        peer->id, to_target_selector(*target), native, session_id));
}

}  // extern "C"
