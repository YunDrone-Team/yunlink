/**
 * @file include/yunlink/core/semantic/state_types.hpp
 * @brief Semantic state, event and bulk descriptor models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_STATE_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_STATE_TYPES_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "yunlink/core/semantic/message_ids.hpp"
#include "yunlink/core/types.hpp"

namespace yunlink {

struct VehicleCoreState {
    bool armed = false;
    uint8_t nav_mode = 0;
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float battery_percent = 0.0F;
};

struct Vector3f {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};

struct Vector2f {
    float x = 0.0F;
    float y = 0.0F;
};

struct HeaderSnapshot {
    std::string frame_id;
    uint64_t stamp_ns = 0;
};

struct HostSystemSnapshot {
    HeaderSnapshot header;
    float cpu_percent = 0.0F;
    float memory_percent = 0.0F;
    uint32_t sample_period_ms = 1000;
    std::string component_kind;
    std::vector<std::string> active_components;
};

struct Quaternionf {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    float w = 1.0F;
};

struct GeoPointSnapshot {
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    double altitude_m = 0.0;
};

struct PoseSnapshot {
    Vector3f position_m;
    Quaternionf orientation;
};

struct TwistSnapshot {
    Vector3f linear_mps;
    Vector3f angular_radps;
};

struct TransformSnapshot {
    HeaderSnapshot header;
    std::string child_frame_id;
    Vector3f translation_m;
    Quaternionf rotation;
};

struct OdometrySnapshot {
    HeaderSnapshot header;
    std::string child_frame_id;
    PoseSnapshot pose;
    std::array<double, 36> pose_covariance{};
    TwistSnapshot twist;
    std::array<double, 36> twist_covariance{};
};

struct UavControlCmdSnapshot {
    HeaderSnapshot header;
    uint8_t cmd_source = 0;
    uint8_t control_cmd = 0;
    Vector3f desired_pos_m;
    Vector3f desired_vel_mps;
    Vector3f desired_acc_mps2;
    Vector3f desired_jerk;
    Vector2f desired_body_xy_pos_m;
    Vector2f desired_body_xy_vel_mps;
    float fixed_height_m = 0.0F;
    uint8_t yaw_mode = 0;
    float desired_yaw_rad = 0.0F;
    float desired_yaw_rate_radps = 0.0F;
};

struct PositionTargetSnapshot {
    HeaderSnapshot header;
    uint8_t coordinate_frame = 0;
    uint16_t type_mask = 0;
    Vector3f position_m;
    Vector3f velocity_mps;
    Vector3f acceleration_or_force;
    float yaw_rad = 0.0F;
    float yaw_rate_radps = 0.0F;
};

struct AttitudeTargetSnapshot {
    HeaderSnapshot header;
    uint8_t type_mask = 0;
    Quaternionf orientation;
    Vector3f body_rate_radps;
    float thrust = 0.0F;
};

struct Px4StateSnapshot {
    HeaderSnapshot header;
    bool connected = false;
    bool rc_available = false;
    bool armed = false;
    std::string flight_mode;
    uint8_t system_status = 0;
    uint8_t landed_state = 0;
    float battery_voltage_v = 0.0F;
    float battery_current_a = 0.0F;
    float battery_percentage = 0.0F;
    uint16_t fcu_load = 0;
    PoseSnapshot external_pose;
    TwistSnapshot external_velocity;
    PoseSnapshot local_pose;
    TwistSnapshot local_velocity;
    uint8_t setpoint_coordinate_frame = 0;
    uint16_t setpoint_local_type_mask = 0;
    Vector3f pos_setpoint_m;
    Vector3f vel_setpoint_mps;
    Vector3f acc_setpoint_mps2;
    float yaw_setpoint_rad = 0.0F;
    float yaw_rate_setpoint_radps = 0.0F;
    uint16_t setpoint_att_type_mask = 0;
    Quaternionf orientation_setpoint;
    Vector3f body_rate_setpoint_radps;
    float thrust_setpoint = 0.0F;
    uint8_t satellites = 0;
    int8_t gps_status = 0;
    double latitude_deg = 0.0;
    double longitude_deg = 0.0;
    double altitude_m = 0.0;
    double latitude_raw_deg = 0.0;
    double longitude_raw_deg = 0.0;
    double altitude_amsl_m = 0.0;
};

struct OdomStatusSnapshot {
    std::string external_source_name;
    uint8_t external_source_id = 0;
    std::string localization_mode_name;
    uint8_t localization_mode = 0;
    bool has_odometry = false;
    bool has_relocalization = false;
    bool odom_timeout = false;
    bool relocalization_data_valid = false;
    uint32_t last_odometry_age_ms = 0;
    std::string global_frame_id;
    std::string local_frame_id;
    std::string base_frame_id;
};

struct UavControlFsmStateSnapshot {
    double takeoff_relative_height_m = 0.0;
    double takeoff_max_velocity_mps = 0.0;
    uint8_t land_type = 0;
    double land_max_velocity_mps = 0.0;
    Vector3f home_point_m;
    uint8_t control_command = 0;
    uint8_t yunlink_fsm_state = 0;
};

struct UavControllerStateSnapshot {
    uint8_t reference_frame = 0;
    uint8_t controller_type = 0;
    Vector3f desired_position_m;
    Vector3f desired_velocity_mps;
    Vector3f current_position_m;
    Vector3f current_velocity_mps;
    Vector3f position_error_m;
    Vector3f velocity_error_mps;
    double desired_yaw_rad = 0.0;
    double current_yaw_rad = 0.0;
    double yaw_error_rad = 0.0;
    double thrust_from_px4 = 0.0;
    double thrust_from_controller = 0.0;
};

struct GimbalParamsSnapshot {
    uint8_t stream_type = 0;
    uint8_t encoding_type = 0;
    uint16_t resolution_width = 0;
    uint16_t resolution_height = 0;
    uint16_t bitrate_kbps = 0;
    float frame_rate = 0.0F;
};

struct LocalOdomSnapshot {
    HeaderSnapshot header;
    std::string child_frame_id;
    PoseSnapshot pose;
    std::array<double, 36> pose_covariance{};
    TwistSnapshot twist;
    std::array<double, 36> twist_covariance{};
};

struct UavControlStateSnapshot {
    HeaderSnapshot header;
    std::string agent_name;
    uint8_t agent_id = 0;
    uint8_t controller_types = 0;
    double takeoff_relative_height_m = 0.0;
    double takeoff_max_velocity_mps = 0.0;
    uint8_t land_type = 0;
    double land_max_velocity_mps = 0.0;
    Vector3f home_point_m;
    uint8_t control_state = 0;
    UavControlCmdSnapshot last_cmd;
    OdometrySnapshot self_odom;
    bool odometry_lost = false;
    bool odometry_valid = false;
    uint8_t controller_output_type = 0;
    PositionTargetSnapshot position_target;
    AttitudeTargetSnapshot attitude_target;
};

struct OdomStateSnapshot {
    HeaderSnapshot header;
    uint8_t external_source = 0;
    std::string subtopic_name_external_odom;
    bool odometry_valid = false;
    float odometry_update_hz = 0.0F;
    std::string subtopic_name_external_relocalization;
    std::string pubtopic_name_local_odom;
    std::string pubtopic_name_global_odom;
    OdometrySnapshot local_odom;
    OdometrySnapshot global_odom;
    std::string world_frame_name;
    std::string global_frame_name;
    std::string local_frame_name;
    std::string base_frame_name;
    TransformSnapshot world_to_global_tf;
    TransformSnapshot global_to_local_tf;
    TransformSnapshot local_to_base_tf;
};

struct VehicleEvent {
    VehicleEventKind kind = VehicleEventKind::kInfo;
    uint8_t severity = 0;
    std::string detail;
};

struct BulkChannelDescriptor {
    uint32_t channel_id = 0;
    BulkStreamType stream_type = BulkStreamType::kPointCloud;
    BulkChannelState state = BulkChannelState::kReady;
    std::string uri;
    uint32_t mtu_bytes = 0;
    bool reliable = false;
    std::string detail;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_STATE_TYPES_HPP
