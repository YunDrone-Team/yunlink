/**
 * @file src/core/semantic_messages_state_snapshots.cpp
 * @brief 新增状态快照 payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

namespace {

void write_vec3(BufferWriter& writer, const Vector3f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
}

bool read_vec3(BufferReader& reader, Vector3f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) && reader.read_float(&out->z);
}

}  // namespace

ByteBuffer encode_payload(const LocalOdomSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_vec3(writer, payload.position_m);
        writer.write_float(payload.orientation_x);
        writer.write_float(payload.orientation_y);
        writer.write_float(payload.orientation_z);
        writer.write_float(payload.orientation_w);
        write_vec3(writer, payload.linear_velocity_mps);
    });
}

bool decode_payload(const ByteBuffer& bytes, LocalOdomSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, LocalOdomSnapshot* out) {
        return read_vec3(reader, &out->position_m) && reader.read_float(&out->orientation_x) &&
               reader.read_float(&out->orientation_y) && reader.read_float(&out->orientation_z) &&
               reader.read_float(&out->orientation_w) &&
               read_vec3(reader, &out->linear_velocity_mps);
    });
}

ByteBuffer encode_payload(const MavrosStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.connected);
        writer.write_bool(payload.armed);
        writer.write_bool(payload.guided);
        writer.write_string(payload.mode_name);
        writer.write_u8(payload.system_status);
    });
}

bool decode_payload(const ByteBuffer& bytes, MavrosStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, MavrosStateSnapshot* out) {
        return reader.read_bool(&out->connected) && reader.read_bool(&out->armed) &&
               reader.read_bool(&out->guided) && reader.read_string(&out->mode_name) &&
               reader.read_u8(&out->system_status);
    });
}

ByteBuffer encode_payload(const UavControlStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(payload.controller_types);
        writer.write_double(payload.takeoff_relative_height_m);
        writer.write_double(payload.takeoff_max_velocity_mps);
        writer.write_u8(payload.land_type);
        writer.write_double(payload.land_max_velocity_mps);
        write_vec3(writer, payload.home_point_m);
        writer.write_u8(payload.control_state);
        writer.write_u8(payload.last_control_cmd);
        writer.write_u8(payload.last_cmd_source);
        writer.write_bool(payload.odometry_lost);
        writer.write_bool(payload.odometry_valid);
        writer.write_float(payload.self_odom_z_m);
    });
}

bool decode_payload(const ByteBuffer& bytes, UavControlStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, UavControlStateSnapshot* out) {
        return reader.read_u8(&out->controller_types) &&
               reader.read_double(&out->takeoff_relative_height_m) &&
               reader.read_double(&out->takeoff_max_velocity_mps) &&
               reader.read_u8(&out->land_type) && reader.read_double(&out->land_max_velocity_mps) &&
               read_vec3(reader, &out->home_point_m) && reader.read_u8(&out->control_state) &&
               reader.read_u8(&out->last_control_cmd) && reader.read_u8(&out->last_cmd_source) &&
               reader.read_bool(&out->odometry_lost) && reader.read_bool(&out->odometry_valid) &&
               reader.read_float(&out->self_odom_z_m);
    });
}

ByteBuffer encode_payload(const OdomStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(payload.external_source);
        writer.write_string(payload.subtopic_name_external_odom);
        writer.write_bool(payload.odometry_valid);
        writer.write_float(payload.odometry_update_hz);
        writer.write_string(payload.subtopic_name_external_relocalization);
        writer.write_string(payload.pubtopic_name_local_odom);
        writer.write_string(payload.pubtopic_name_global_odom);
        writer.write_string(payload.world_frame_name);
        writer.write_string(payload.global_frame_name);
        writer.write_string(payload.local_frame_name);
        writer.write_string(payload.base_frame_name);
    });
}

bool decode_payload(const ByteBuffer& bytes, OdomStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, OdomStateSnapshot* out) {
        return reader.read_u8(&out->external_source) &&
               reader.read_string(&out->subtopic_name_external_odom) &&
               reader.read_bool(&out->odometry_valid) &&
               reader.read_float(&out->odometry_update_hz) &&
               reader.read_string(&out->subtopic_name_external_relocalization) &&
               reader.read_string(&out->pubtopic_name_local_odom) &&
               reader.read_string(&out->pubtopic_name_global_odom) &&
               reader.read_string(&out->world_frame_name) &&
               reader.read_string(&out->global_frame_name) &&
               reader.read_string(&out->local_frame_name) &&
               reader.read_string(&out->base_frame_name);
    });
}

}  // namespace yunlink
