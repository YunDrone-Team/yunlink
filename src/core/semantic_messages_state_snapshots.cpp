/**
 * @file src/core/semantic_messages_state_snapshots.cpp
 * @brief 新增状态快照 payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

namespace {

void write_header(BufferWriter& writer, const HeaderSnapshot& value) {
    writer.write_u32(value.seq);
    writer.write_double(value.stamp_sec);
    writer.write_string(value.frame_id);
}

bool read_header(BufferReader& reader, HeaderSnapshot* out) {
    return reader.read_u32(&out->seq) && reader.read_double(&out->stamp_sec) &&
           reader.read_string(&out->frame_id);
}

void write_vec2(BufferWriter& writer, const Vector2f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
}

bool read_vec2(BufferReader& reader, Vector2f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y);
}

void write_vec3(BufferWriter& writer, const Vector3f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
}

bool read_vec3(BufferReader& reader, Vector3f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) && reader.read_float(&out->z);
}

void write_quat(BufferWriter& writer, const Quaternionf& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
    writer.write_float(value.w);
}

bool read_quat(BufferReader& reader, Quaternionf* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) &&
           reader.read_float(&out->z) && reader.read_float(&out->w);
}

void write_geo(BufferWriter& writer, const GeoPointSnapshot& value) {
    writer.write_double(value.latitude_deg);
    writer.write_double(value.longitude_deg);
    writer.write_double(value.altitude_m);
}

bool read_geo(BufferReader& reader, GeoPointSnapshot* out) {
    return reader.read_double(&out->latitude_deg) && reader.read_double(&out->longitude_deg) &&
           reader.read_double(&out->altitude_m);
}

void write_pose(BufferWriter& writer, const PoseSnapshot& value) {
    write_vec3(writer, value.position_m);
    write_quat(writer, value.orientation);
}

bool read_pose(BufferReader& reader, PoseSnapshot* out) {
    return read_vec3(reader, &out->position_m) && read_quat(reader, &out->orientation);
}

void write_twist(BufferWriter& writer, const TwistSnapshot& value) {
    write_vec3(writer, value.linear_mps);
    write_vec3(writer, value.angular_radps);
}

bool read_twist(BufferReader& reader, TwistSnapshot* out) {
    return read_vec3(reader, &out->linear_mps) && read_vec3(reader, &out->angular_radps);
}

void write_transform(BufferWriter& writer, const TransformSnapshot& value) {
    write_header(writer, value.header);
    writer.write_string(value.child_frame_id);
    write_vec3(writer, value.translation_m);
    write_quat(writer, value.rotation);
}

bool read_transform(BufferReader& reader, TransformSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_string(&out->child_frame_id) &&
           read_vec3(reader, &out->translation_m) && read_quat(reader, &out->rotation);
}

void write_covariance(BufferWriter& writer, const std::array<double, 36>& value) {
    for (double item : value) {
        writer.write_double(item);
    }
}

bool read_covariance(BufferReader& reader, std::array<double, 36>* out) {
    for (double& item : *out) {
        if (!reader.read_double(&item)) {
            return false;
        }
    }
    return true;
}

void write_odometry(BufferWriter& writer, const OdometrySnapshot& value) {
    write_header(writer, value.header);
    writer.write_string(value.child_frame_id);
    write_pose(writer, value.pose);
    write_covariance(writer, value.pose_covariance);
    write_twist(writer, value.twist);
    write_covariance(writer, value.twist_covariance);
}

bool read_odometry(BufferReader& reader, OdometrySnapshot* out) {
    return read_header(reader, &out->header) && reader.read_string(&out->child_frame_id) &&
           read_pose(reader, &out->pose) && read_covariance(reader, &out->pose_covariance) &&
           read_twist(reader, &out->twist) && read_covariance(reader, &out->twist_covariance);
}

void write_uav_control_cmd(BufferWriter& writer, const UavControlCmdSnapshot& value) {
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
    write_geo(writer, value.desired_wgs84_pos);
    writer.write_u8(value.yaw_mode);
    writer.write_float(value.desired_yaw_rad);
    writer.write_float(value.desired_yaw_rate_radps);
}

bool read_uav_control_cmd(BufferReader& reader, UavControlCmdSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_u8(&out->cmd_source) &&
           reader.read_u8(&out->control_cmd) && read_vec3(reader, &out->desired_pos_m) &&
           read_vec3(reader, &out->desired_vel_mps) &&
           read_vec3(reader, &out->desired_acc_mps2) && read_vec3(reader, &out->desired_jerk) &&
           read_vec2(reader, &out->desired_body_xy_pos_m) &&
           read_vec2(reader, &out->desired_body_xy_vel_mps) &&
           reader.read_float(&out->fixed_height_m) && read_geo(reader, &out->desired_wgs84_pos) &&
           reader.read_u8(&out->yaw_mode) && reader.read_float(&out->desired_yaw_rad) &&
           reader.read_float(&out->desired_yaw_rate_radps);
}

void write_position_target(BufferWriter& writer, const PositionTargetSnapshot& value) {
    write_header(writer, value.header);
    writer.write_u8(value.coordinate_frame);
    writer.write_u16(value.type_mask);
    write_vec3(writer, value.position_m);
    write_vec3(writer, value.velocity_mps);
    write_vec3(writer, value.acceleration_or_force);
    writer.write_float(value.yaw_rad);
    writer.write_float(value.yaw_rate_radps);
}

bool read_position_target(BufferReader& reader, PositionTargetSnapshot* out) {
    return read_header(reader, &out->header) && reader.read_u8(&out->coordinate_frame) &&
           reader.read_u16(&out->type_mask) && read_vec3(reader, &out->position_m) &&
           read_vec3(reader, &out->velocity_mps) &&
           read_vec3(reader, &out->acceleration_or_force) && reader.read_float(&out->yaw_rad) &&
           reader.read_float(&out->yaw_rate_radps);
}

void write_attitude_target(BufferWriter& writer, const AttitudeTargetSnapshot& value) {
    write_header(writer, value.header);
    writer.write_u8(static_cast<uint8_t>(value.type_mask & 0xFFU));
    write_quat(writer, value.orientation);
    write_vec3(writer, value.body_rate_radps);
    writer.write_float(value.thrust);
}

bool read_attitude_target(BufferReader& reader, AttitudeTargetSnapshot* out) {
    uint8_t mask = 0;
    if (!read_header(reader, &out->header) || !reader.read_u8(&mask)) {
        return false;
    }
    out->type_mask = mask;
    return read_quat(reader, &out->orientation) && read_vec3(reader, &out->body_rate_radps) &&
           reader.read_float(&out->thrust);
}

}  // namespace

ByteBuffer encode_payload(const LocalOdomSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_string(payload.child_frame_id);
        write_pose(writer, payload.pose);
        write_covariance(writer, payload.pose_covariance);
        write_twist(writer, payload.twist);
        write_covariance(writer, payload.twist_covariance);
    });
}

bool decode_payload(const ByteBuffer& bytes, LocalOdomSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, LocalOdomSnapshot* out) {
        return read_header(reader, &out->header) && reader.read_string(&out->child_frame_id) &&
               read_pose(reader, &out->pose) && read_covariance(reader, &out->pose_covariance) &&
               read_twist(reader, &out->twist) && read_covariance(reader, &out->twist_covariance);
    });
}

ByteBuffer encode_payload(const MavrosStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_bool(payload.connected);
        writer.write_bool(payload.armed);
        writer.write_bool(payload.guided);
        writer.write_bool(payload.manual_input);
        writer.write_string(payload.mode);
        writer.write_u8(payload.system_status);
    });
}

bool decode_payload(const ByteBuffer& bytes, MavrosStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, MavrosStateSnapshot* out) {
        return read_header(reader, &out->header) && reader.read_bool(&out->connected) &&
               reader.read_bool(&out->armed) && reader.read_bool(&out->guided) &&
               reader.read_bool(&out->manual_input) && reader.read_string(&out->mode) &&
               reader.read_u8(&out->system_status);
    });
}

ByteBuffer encode_payload(const UavControlStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_string(payload.agent_name);
        writer.write_u8(payload.agent_id);
        writer.write_u8(payload.controller_types);
        writer.write_double(payload.takeoff_relative_height_m);
        writer.write_double(payload.takeoff_max_velocity_mps);
        writer.write_u8(payload.land_type);
        writer.write_double(payload.land_max_velocity_mps);
        write_vec3(writer, payload.home_point_m);
        writer.write_u8(payload.control_state);
        write_uav_control_cmd(writer, payload.last_cmd);
        write_odometry(writer, payload.self_odom);
        writer.write_bool(payload.odometry_lost);
        writer.write_bool(payload.odometry_valid);
        writer.write_u8(payload.controller_output_type);
        write_position_target(writer, payload.position_target);
        write_attitude_target(writer, payload.attitude_target);
    });
}

bool decode_payload(const ByteBuffer& bytes, UavControlStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, UavControlStateSnapshot* out) {
        return read_header(reader, &out->header) && reader.read_string(&out->agent_name) &&
               reader.read_u8(&out->agent_id) && reader.read_u8(&out->controller_types) &&
               reader.read_double(&out->takeoff_relative_height_m) &&
               reader.read_double(&out->takeoff_max_velocity_mps) &&
               reader.read_u8(&out->land_type) && reader.read_double(&out->land_max_velocity_mps) &&
               read_vec3(reader, &out->home_point_m) && reader.read_u8(&out->control_state) &&
               read_uav_control_cmd(reader, &out->last_cmd) &&
               read_odometry(reader, &out->self_odom) && reader.read_bool(&out->odometry_lost) &&
               reader.read_bool(&out->odometry_valid) &&
               reader.read_u8(&out->controller_output_type) &&
               read_position_target(reader, &out->position_target) &&
               read_attitude_target(reader, &out->attitude_target);
    });
}

ByteBuffer encode_payload(const OdomStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_u8(payload.external_source);
        writer.write_string(payload.subtopic_name_external_odom);
        writer.write_bool(payload.odometry_valid);
        writer.write_float(payload.odometry_update_hz);
        writer.write_string(payload.subtopic_name_external_relocalization);
        writer.write_string(payload.pubtopic_name_local_odom);
        writer.write_string(payload.pubtopic_name_global_odom);
        write_odometry(writer, payload.local_odom);
        write_odometry(writer, payload.global_odom);
        writer.write_string(payload.world_frame_name);
        writer.write_string(payload.global_frame_name);
        writer.write_string(payload.local_frame_name);
        writer.write_string(payload.base_frame_name);
        write_transform(writer, payload.world_to_global_tf);
        write_transform(writer, payload.global_to_local_tf);
        write_transform(writer, payload.local_to_base_tf);
    });
}

bool decode_payload(const ByteBuffer& bytes, OdomStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, OdomStateSnapshot* out) {
        return read_header(reader, &out->header) && reader.read_u8(&out->external_source) &&
               reader.read_string(&out->subtopic_name_external_odom) &&
               reader.read_bool(&out->odometry_valid) &&
               reader.read_float(&out->odometry_update_hz) &&
               reader.read_string(&out->subtopic_name_external_relocalization) &&
               reader.read_string(&out->pubtopic_name_local_odom) &&
               reader.read_string(&out->pubtopic_name_global_odom) &&
               read_odometry(reader, &out->local_odom) &&
               read_odometry(reader, &out->global_odom) &&
               reader.read_string(&out->world_frame_name) &&
               reader.read_string(&out->global_frame_name) &&
               reader.read_string(&out->local_frame_name) &&
               reader.read_string(&out->base_frame_name) &&
               read_transform(reader, &out->world_to_global_tf) &&
               read_transform(reader, &out->global_to_local_tf) &&
               read_transform(reader, &out->local_to_base_tf);
    });
}

}  // namespace yunlink
