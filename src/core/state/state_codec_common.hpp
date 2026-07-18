/**
 * @file src/core/state/state_codec_common.hpp
 * @brief Shared helpers for state and snapshot payload codec implementations.
 */

#ifndef YUNLINK_CORE_STATE_STATE_CODEC_COMMON_HPP
#define YUNLINK_CORE_STATE_STATE_CODEC_COMMON_HPP

#include "../semantic_codec_io.hpp"

namespace yunlink::state_codec_detail {

inline bool valid_vehicle_event_kind(uint8_t value) {
    return value >= static_cast<uint8_t>(VehicleEventKind::kInfo) &&
           value <= static_cast<uint8_t>(VehicleEventKind::kFault);
}

inline bool valid_bulk_stream_type(uint8_t value) {
    return value >= static_cast<uint8_t>(BulkStreamType::kPointCloud) &&
           value <= static_cast<uint8_t>(BulkStreamType::kVideo);
}

inline bool valid_bulk_channel_state(uint8_t value) {
    return value >= static_cast<uint8_t>(BulkChannelState::kReady) &&
           value <= static_cast<uint8_t>(BulkChannelState::kClosed);
}

inline void write_header(BufferWriter& writer, const HeaderSnapshot& value) {
    writer.write_string(value.frame_id);
    writer.write_u64(value.stamp_ns);
}

inline bool read_header(BufferReader& reader, HeaderSnapshot* out) {
    return reader.read_string(&out->frame_id) && reader.read_u64(&out->stamp_ns);
}

inline void write_vec2(BufferWriter& writer, const Vector2f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
}

inline bool read_vec2(BufferReader& reader, Vector2f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y);
}

inline void write_vec3(BufferWriter& writer, const Vector3f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
}

inline bool read_vec3(BufferReader& reader, Vector3f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) && reader.read_float(&out->z);
}

inline void write_quat(BufferWriter& writer, const Quaternionf& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
    writer.write_float(value.w);
}

inline bool read_quat(BufferReader& reader, Quaternionf* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) && reader.read_float(&out->z) &&
           reader.read_float(&out->w);
}

inline void write_geo(BufferWriter& writer, const GeoPointSnapshot& value) {
    writer.write_double(value.latitude_deg);
    writer.write_double(value.longitude_deg);
    writer.write_double(value.altitude_m);
}

inline bool read_geo(BufferReader& reader, GeoPointSnapshot* out) {
    return reader.read_double(&out->latitude_deg) && reader.read_double(&out->longitude_deg) &&
           reader.read_double(&out->altitude_m);
}

inline void write_pose(BufferWriter& writer, const PoseSnapshot& value) {
    write_vec3(writer, value.position_m);
    write_quat(writer, value.orientation);
}

inline bool read_pose(BufferReader& reader, PoseSnapshot* out) {
    return read_vec3(reader, &out->position_m) && read_quat(reader, &out->orientation);
}

inline void write_twist(BufferWriter& writer, const TwistSnapshot& value) {
    write_vec3(writer, value.linear_mps);
    write_vec3(writer, value.angular_radps);
}

inline bool read_twist(BufferReader& reader, TwistSnapshot* out) {
    return read_vec3(reader, &out->linear_mps) && read_vec3(reader, &out->angular_radps);
}

inline void write_transform(BufferWriter& writer, const TransformSnapshot& value) {
    write_header(writer, value.header);
    writer.write_string(value.child_frame_id);
    write_vec3(writer, value.translation_m);
    write_quat(writer, value.rotation);
}

inline bool read_transform(BufferReader& reader, TransformSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_string(&out->child_frame_id) &&
           read_vec3(reader, &out->translation_m) && read_quat(reader, &out->rotation);
}

inline void write_topic_diagnostic(BufferWriter& writer,
                                   const SunrayTopicDiagnosticSnapshot& value) {
    writer.write_string(value.key);
    writer.write_string(value.topic);
    writer.write_bool(value.configured);
    writer.write_bool(value.has_message);
    writer.write_u32(value.publisher_count);
    writer.write_u64(value.message_count);
    writer.write_float(value.hz);
    writer.write_u32(value.age_ms);
    writer.write_bool(value.stale);
    writer.write_string(value.status);
    writer.write_string(value.detail);
    writer.write_string(value.last_transition);
    writer.write_u32(value.last_transition_age_ms);
    writer.write_u64(value.publish_fail_count);
    writer.write_float(value.expected_min_hz);
    writer.write_bool(value.sparse);
}

inline bool read_topic_diagnostic(BufferReader& reader, SunrayTopicDiagnosticSnapshot* out) {
    return reader.read_string(&out->key) && reader.read_string(&out->topic) &&
           reader.read_bool(&out->configured) && reader.read_bool(&out->has_message) &&
           reader.read_u32(&out->publisher_count) && reader.read_u64(&out->message_count) &&
           reader.read_float(&out->hz) && reader.read_u32(&out->age_ms) &&
           reader.read_bool(&out->stale) && reader.read_string(&out->status) &&
           reader.read_string(&out->detail) && reader.read_string(&out->last_transition) &&
           reader.read_u32(&out->last_transition_age_ms) &&
           reader.read_u64(&out->publish_fail_count) && reader.read_float(&out->expected_min_hz) &&
           reader.read_bool(&out->sparse);
}

inline void write_covariance(BufferWriter& writer, const std::array<double, 36>& value) {
    for (double item : value) {
        writer.write_double(item);
    }
}

inline bool read_covariance(BufferReader& reader, std::array<double, 36>* out) {
    for (double& item : *out) {
        if (!reader.read_double(&item)) {
            return false;
        }
    }
    return true;
}

inline void write_odometry(BufferWriter& writer, const OdometrySnapshot& value) {
    write_header(writer, value.header);
    writer.write_string(value.child_frame_id);
    write_pose(writer, value.pose);
    write_covariance(writer, value.pose_covariance);
    write_twist(writer, value.twist);
    write_covariance(writer, value.twist_covariance);
}

inline bool read_odometry(BufferReader& reader, OdometrySnapshot* out) {
    return read_header(reader, &out->header) && reader.read_string(&out->child_frame_id) &&
           read_pose(reader, &out->pose) && read_covariance(reader, &out->pose_covariance) &&
           read_twist(reader, &out->twist) && read_covariance(reader, &out->twist_covariance);
}

inline void write_uav_control_cmd(BufferWriter& writer, const UavControlCmdSnapshot& value) {
    write_header(writer, value.header);
    writer.write_u8(value.cmd_source);
    writer.write_u8(value.control_cmd);
    write_vec3(writer, value.desired_pos_m);
    write_vec3(writer, value.desired_vel_mps);
    write_vec3(writer, value.desired_acc_mps2);
    write_vec3(writer, value.desired_jerk);
    write_vec2(writer, value.desired_body_xy_pos_m);
    write_vec2(writer, value.desired_body_xy_vel_mps);
    writer.write_float(value.fixed_height_m);
    writer.write_u8(value.yaw_mode);
    writer.write_float(value.desired_yaw_rad);
    writer.write_float(value.desired_yaw_rate_radps);
}

inline bool read_uav_control_cmd(BufferReader& reader, UavControlCmdSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_u8(&out->cmd_source) &&
           reader.read_u8(&out->control_cmd) && read_vec3(reader, &out->desired_pos_m) &&
           read_vec3(reader, &out->desired_vel_mps) && read_vec3(reader, &out->desired_acc_mps2) &&
           read_vec3(reader, &out->desired_jerk) &&
           read_vec2(reader, &out->desired_body_xy_pos_m) &&
           read_vec2(reader, &out->desired_body_xy_vel_mps) &&
           reader.read_float(&out->fixed_height_m) && reader.read_u8(&out->yaw_mode) &&
           reader.read_float(&out->desired_yaw_rad) &&
           reader.read_float(&out->desired_yaw_rate_radps);
}

inline void write_position_target(BufferWriter& writer, const PositionTargetSnapshot& value) {
    write_header(writer, value.header);
    writer.write_u8(value.coordinate_frame);
    writer.write_u16(value.type_mask);
    write_vec3(writer, value.position_m);
    write_vec3(writer, value.velocity_mps);
    write_vec3(writer, value.acceleration_or_force);
    writer.write_float(value.yaw_rad);
    writer.write_float(value.yaw_rate_radps);
}

inline bool read_position_target(BufferReader& reader, PositionTargetSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_u8(&out->coordinate_frame) &&
           reader.read_u16(&out->type_mask) && read_vec3(reader, &out->position_m) &&
           read_vec3(reader, &out->velocity_mps) &&
           read_vec3(reader, &out->acceleration_or_force) && reader.read_float(&out->yaw_rad) &&
           reader.read_float(&out->yaw_rate_radps);
}

inline void write_attitude_target(BufferWriter& writer, const AttitudeTargetSnapshot& value) {
    write_header(writer, value.header);
    writer.write_u8(static_cast<uint8_t>(value.type_mask & 0xFFU));
    write_quat(writer, value.orientation);
    write_vec3(writer, value.body_rate_radps);
    writer.write_float(value.thrust);
}

inline bool read_attitude_target(BufferReader& reader, AttitudeTargetSnapshot* out) {
    uint8_t mask = 0;
    if (!read_header(reader, &out->header) || !reader.read_u8(&mask)) {
        return false;
    }
    out->type_mask = mask;
    return read_quat(reader, &out->orientation) && read_vec3(reader, &out->body_rate_radps) &&
           reader.read_float(&out->thrust);
}

inline bool valid_command_kind(uint16_t value) {
    return value <= static_cast<uint16_t>(CommandKind::kUavControl);
}

inline bool valid_command_execution_state(uint8_t value) {
    return value <= static_cast<uint8_t>(CommandExecutionState::kTimeout);
}

}  // namespace yunlink::state_codec_detail

#endif  // YUNLINK_CORE_STATE_STATE_CODEC_COMMON_HPP
