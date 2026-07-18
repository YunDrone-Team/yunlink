/** @file @brief Complete UAV control command schema-1 codec contract. */

#include <iostream>

#include "yunlink/core/semantic_messages.hpp"

int main() {
    static_assert(yunlink::MessageTraits<yunlink::UavControlCommand>::kSchemaVersion == 1,
                  "UAV control command must remain in schema 1");

    yunlink::UavControlCommand input{};
    input.control_cmd = 4;
    input.desired_position_m = {1.0F, 2.0F, 3.0F};
    input.desired_velocity_mps = {4.0F, 5.0F, 6.0F};
    input.desired_acceleration_mps2 = {7.0F, 8.0F, 9.0F};
    input.desired_body_xy_position_m = {10.0F, 11.0F};
    input.desired_body_xy_velocity_mps = {12.0F, 13.0F};
    input.fixed_height_m = 14.0F;
    input.yaw_mode = 2;
    input.desired_yaw_rad = 1.5F;
    input.desired_yaw_rate_radps = -0.25F;
    input.controller_type = 1;

    const auto bytes = yunlink::encode_payload(input);
    yunlink::UavControlCommand output{};
    if (bytes.empty() || !yunlink::decode_payload(bytes, &output) ||
        output.control_cmd != input.control_cmd ||
        output.desired_position_m.x != input.desired_position_m.x ||
        output.desired_position_m.y != input.desired_position_m.y ||
        output.desired_position_m.z != input.desired_position_m.z ||
        output.desired_velocity_mps.x != input.desired_velocity_mps.x ||
        output.desired_velocity_mps.y != input.desired_velocity_mps.y ||
        output.desired_velocity_mps.z != input.desired_velocity_mps.z ||
        output.desired_acceleration_mps2.x != input.desired_acceleration_mps2.x ||
        output.desired_acceleration_mps2.y != input.desired_acceleration_mps2.y ||
        output.desired_acceleration_mps2.z != input.desired_acceleration_mps2.z ||
        output.desired_body_xy_position_m.x != input.desired_body_xy_position_m.x ||
        output.desired_body_xy_position_m.y != input.desired_body_xy_position_m.y ||
        output.desired_body_xy_velocity_mps.x != input.desired_body_xy_velocity_mps.x ||
        output.desired_body_xy_velocity_mps.y != input.desired_body_xy_velocity_mps.y ||
        output.fixed_height_m != input.fixed_height_m || output.yaw_mode != input.yaw_mode ||
        output.desired_yaw_rad != input.desired_yaw_rad ||
        output.desired_yaw_rate_radps != input.desired_yaw_rate_radps ||
        output.controller_type != input.controller_type) {
        std::cerr << "UAV control command round-trip failed\n";
        return 1;
    }

    auto truncated = bytes;
    truncated.pop_back();
    if (yunlink::decode_payload(truncated, &output)) {
        std::cerr << "truncated UAV control command accepted\n";
        return 2;
    }
    return 0;
}
