/**
 * @file include/yunlink/core/semantic/command_types.hpp
 * @brief Semantic command payload models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_COMMAND_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_COMMAND_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "yunlink/core/semantic/message_ids.hpp"
#include "yunlink/core/types.hpp"

namespace yunlink {

struct TakeoffCommand {
    uint8_t reserved = 0;
};

struct LandCommand {
    uint8_t reserved = 0;
};

struct ReturnCommand {
    uint8_t reserved = 0;
};

struct GotoCommand {
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float yaw_rad = 0.0F;
};

struct VelocitySetpointCommand {
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float yaw_rate_radps = 0.0F;
    bool body_frame = false;
};

/** A complete UAV control command independent of any vehicle-side middleware. */
struct UavControlCommand {
    uint8_t control_cmd = 0;
    Vector3f desired_position_m;
    Vector3f desired_velocity_mps;
    Vector3f desired_acceleration_mps2;
    Vector2f desired_body_xy_position_m;
    Vector2f desired_body_xy_velocity_mps;
    float fixed_height_m = 0.0F;
    uint8_t yaw_mode = 0;
    float desired_yaw_rad = 0.0F;
    float desired_yaw_rate_radps = 0.0F;
    uint8_t controller_type = 0;
};

/** Complete ground-vehicle control command independent of vehicle-side middleware. */
struct UgvControlCommand {
    uint8_t control_cmd = 0;
    Vector3f desired_position_m;
    Vector3f desired_velocity_mps;
    Vector3f body_linear_velocity_mps;
    Vector3f body_angular_velocity_radps;
    float desired_yaw_rad = 0.0F;
    double desired_wgs84_latitude_deg = 0.0;
    double desired_wgs84_longitude_deg = 0.0;
    double desired_wgs84_altitude_m = 0.0;
};

struct TrajectoryPoint {
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float yaw_rad = 0.0F;
    uint32_t dt_ms = 0;
};

struct TrajectoryChunkCommand {
    uint32_t chunk_index = 0;
    bool final_chunk = false;
    std::vector<TrajectoryPoint> points;
};

struct FormationTaskCommand {
    uint32_t group_id = 0;
    uint8_t formation_shape = 0;
    float spacing_m = 0.0F;
    std::string label;
};

struct CommandResult {
    CommandKind command_kind = CommandKind::kUnknown;
    CommandPhase phase = CommandPhase::kReceived;
    uint16_t result_code = 0;
    uint8_t progress_percent = 0;
    std::string detail;
};

template <typename T> struct TypedMessage {
    SecureEnvelope envelope;
    T payload;
};

using CommandResultView = TypedMessage<CommandResult>;

struct CommandHandle {
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    TargetSelector target;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_COMMAND_TYPES_HPP
