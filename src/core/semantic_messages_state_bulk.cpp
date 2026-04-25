/**
 * @file src/core/semantic_messages_state_bulk.cpp
 * @brief State 与 Bulk payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

namespace {

bool valid_vehicle_event_kind(uint8_t value) {
    return value >= static_cast<uint8_t>(VehicleEventKind::kInfo) &&
           value <= static_cast<uint8_t>(VehicleEventKind::kFault);
}

bool valid_bulk_stream_type(uint8_t value) {
    return value >= static_cast<uint8_t>(BulkStreamType::kPointCloud) &&
           value <= static_cast<uint8_t>(BulkStreamType::kVideo);
}

bool valid_bulk_channel_state(uint8_t value) {
    return value >= static_cast<uint8_t>(BulkChannelState::kReady) &&
           value <= static_cast<uint8_t>(BulkChannelState::kClosed);
}

void write_vec3(BufferWriter& writer, const Vector3f& value) {
    writer.write_float(value.x);
    writer.write_float(value.y);
    writer.write_float(value.z);
}

bool read_vec3(BufferReader& reader, Vector3f* out) {
    return reader.read_float(&out->x) && reader.read_float(&out->y) && reader.read_float(&out->z);
}

}  // namespace

ByteBuffer encode_payload(const VehicleCoreState& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.armed);
        writer.write_u8(payload.nav_mode);
        writer.write_float(payload.x_m);
        writer.write_float(payload.y_m);
        writer.write_float(payload.z_m);
        writer.write_float(payload.vx_mps);
        writer.write_float(payload.vy_mps);
        writer.write_float(payload.vz_mps);
        writer.write_float(payload.battery_percent);
    });
}

bool decode_payload(const ByteBuffer& bytes, VehicleCoreState* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, VehicleCoreState* out) {
        return reader.read_bool(&out->armed) && reader.read_u8(&out->nav_mode) &&
               reader.read_float(&out->x_m) && reader.read_float(&out->y_m) &&
               reader.read_float(&out->z_m) && reader.read_float(&out->vx_mps) &&
               reader.read_float(&out->vy_mps) && reader.read_float(&out->vz_mps) &&
               reader.read_float(&out->battery_percent);
    });
}

ByteBuffer encode_payload(const Px4StateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.connected);
        writer.write_bool(payload.rc_available);
        writer.write_bool(payload.armed);
        writer.write_u8(payload.flight_mode);
        writer.write_string(payload.flight_mode_name);
        writer.write_u8(payload.system_status);
        writer.write_u8(payload.landed_state);
        writer.write_float(payload.battery_percentage);
        write_vec3(writer, payload.local_position_m);
        write_vec3(writer, payload.local_velocity_mps);
        writer.write_float(payload.yaw_setpoint_rad);
        writer.write_float(payload.yaw_rate_setpoint_radps);
        writer.write_u8(payload.satellites);
        writer.write_i8(payload.gps_status);
        writer.write_u8(payload.gps_service);
        writer.write_double(payload.latitude_deg);
        writer.write_double(payload.longitude_deg);
        writer.write_double(payload.altitude_m);
    });
}

bool decode_payload(const ByteBuffer& bytes, Px4StateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, Px4StateSnapshot* out) {
        return reader.read_bool(&out->connected) && reader.read_bool(&out->rc_available) &&
               reader.read_bool(&out->armed) && reader.read_u8(&out->flight_mode) &&
               reader.read_string(&out->flight_mode_name) && reader.read_u8(&out->system_status) &&
               reader.read_u8(&out->landed_state) && reader.read_float(&out->battery_percentage) &&
               read_vec3(reader, &out->local_position_m) &&
               read_vec3(reader, &out->local_velocity_mps) &&
               reader.read_float(&out->yaw_setpoint_rad) &&
               reader.read_float(&out->yaw_rate_setpoint_radps) &&
               reader.read_u8(&out->satellites) && reader.read_i8(&out->gps_status) &&
               reader.read_u8(&out->gps_service) && reader.read_double(&out->latitude_deg) &&
               reader.read_double(&out->longitude_deg) && reader.read_double(&out->altitude_m);
    });
}

ByteBuffer encode_payload(const OdomStatusSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.external_source_name);
        writer.write_u8(payload.external_source_id);
        writer.write_string(payload.localization_mode_name);
        writer.write_u8(payload.localization_mode);
        writer.write_bool(payload.has_odometry);
        writer.write_bool(payload.has_relocalization);
        writer.write_bool(payload.odom_timeout);
        writer.write_bool(payload.relocalization_data_valid);
        writer.write_u32(payload.last_odometry_age_ms);
        writer.write_string(payload.global_frame_id);
        writer.write_string(payload.local_frame_id);
        writer.write_string(payload.base_frame_id);
    });
}

bool decode_payload(const ByteBuffer& bytes, OdomStatusSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, OdomStatusSnapshot* out) {
        return reader.read_string(&out->external_source_name) &&
               reader.read_u8(&out->external_source_id) &&
               reader.read_string(&out->localization_mode_name) &&
               reader.read_u8(&out->localization_mode) && reader.read_bool(&out->has_odometry) &&
               reader.read_bool(&out->has_relocalization) && reader.read_bool(&out->odom_timeout) &&
               reader.read_bool(&out->relocalization_data_valid) &&
               reader.read_u32(&out->last_odometry_age_ms) &&
               reader.read_string(&out->global_frame_id) &&
               reader.read_string(&out->local_frame_id) && reader.read_string(&out->base_frame_id);
    });
}

ByteBuffer encode_payload(const UavControlFsmStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_double(payload.takeoff_relative_height_m);
        writer.write_double(payload.takeoff_max_velocity_mps);
        writer.write_u8(payload.land_type);
        writer.write_double(payload.land_max_velocity_mps);
        write_vec3(writer, payload.home_point_m);
        writer.write_u8(payload.control_command);
        writer.write_u8(payload.yunlink_fsm_state);
    });
}

bool decode_payload(const ByteBuffer& bytes, UavControlFsmStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, UavControlFsmStateSnapshot* out) {
        return reader.read_double(&out->takeoff_relative_height_m) &&
               reader.read_double(&out->takeoff_max_velocity_mps) &&
               reader.read_u8(&out->land_type) && reader.read_double(&out->land_max_velocity_mps) &&
               read_vec3(reader, &out->home_point_m) && reader.read_u8(&out->control_command) &&
               reader.read_u8(&out->yunlink_fsm_state);
    });
}

ByteBuffer encode_payload(const UavControllerStateSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(payload.reference_frame);
        writer.write_u8(payload.controller_type);
        write_vec3(writer, payload.desired_position_m);
        write_vec3(writer, payload.desired_velocity_mps);
        write_vec3(writer, payload.current_position_m);
        write_vec3(writer, payload.current_velocity_mps);
        write_vec3(writer, payload.position_error_m);
        write_vec3(writer, payload.velocity_error_mps);
        writer.write_double(payload.desired_yaw_rad);
        writer.write_double(payload.current_yaw_rad);
        writer.write_double(payload.yaw_error_rad);
        writer.write_double(payload.thrust_from_px4);
        writer.write_double(payload.thrust_from_controller);
    });
}

bool decode_payload(const ByteBuffer& bytes, UavControllerStateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, UavControllerStateSnapshot* out) {
        return reader.read_u8(&out->reference_frame) && reader.read_u8(&out->controller_type) &&
               read_vec3(reader, &out->desired_position_m) &&
               read_vec3(reader, &out->desired_velocity_mps) &&
               read_vec3(reader, &out->current_position_m) &&
               read_vec3(reader, &out->current_velocity_mps) &&
               read_vec3(reader, &out->position_error_m) &&
               read_vec3(reader, &out->velocity_error_mps) &&
               reader.read_double(&out->desired_yaw_rad) &&
               reader.read_double(&out->current_yaw_rad) &&
               reader.read_double(&out->yaw_error_rad) &&
               reader.read_double(&out->thrust_from_px4) &&
               reader.read_double(&out->thrust_from_controller);
    });
}

ByteBuffer encode_payload(const GimbalParamsSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(payload.stream_type);
        writer.write_u8(payload.encoding_type);
        writer.write_u16(payload.resolution_width);
        writer.write_u16(payload.resolution_height);
        writer.write_u16(payload.bitrate_kbps);
        writer.write_float(payload.frame_rate);
    });
}

bool decode_payload(const ByteBuffer& bytes, GimbalParamsSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, GimbalParamsSnapshot* out) {
        return reader.read_u8(&out->stream_type) && reader.read_u8(&out->encoding_type) &&
               reader.read_u16(&out->resolution_width) &&
               reader.read_u16(&out->resolution_height) && reader.read_u16(&out->bitrate_kbps) &&
               reader.read_float(&out->frame_rate);
    });
}

ByteBuffer encode_payload(const VehicleEvent& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(static_cast<uint8_t>(payload.kind));
        writer.write_u8(payload.severity);
        writer.write_string(payload.detail);
    });
}

bool decode_payload(const ByteBuffer& bytes, VehicleEvent* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, VehicleEvent* out) {
        uint8_t kind = 0;
        if (!reader.read_u8(&kind) || !reader.read_u8(&out->severity) ||
            !reader.read_string(&out->detail)) {
            return false;
        }
        if (!valid_vehicle_event_kind(kind)) {
            return false;
        }
        out->kind = static_cast<VehicleEventKind>(kind);
        return true;
    });
}

ByteBuffer encode_payload(const BulkChannelDescriptor& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u32(payload.channel_id);
        writer.write_u8(static_cast<uint8_t>(payload.stream_type));
        writer.write_u8(static_cast<uint8_t>(payload.state));
        writer.write_string(payload.uri);
        writer.write_u32(payload.mtu_bytes);
        writer.write_bool(payload.reliable);
        writer.write_string(payload.detail);
    });
}

bool decode_payload(const ByteBuffer& bytes, BulkChannelDescriptor* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, BulkChannelDescriptor* out) {
        uint8_t type = 0;
        uint8_t state = 0;
        if (!reader.read_u32(&out->channel_id) || !reader.read_u8(&type) ||
            !reader.read_u8(&state) || !reader.read_string(&out->uri) ||
            !reader.read_u32(&out->mtu_bytes) || !reader.read_bool(&out->reliable) ||
            !reader.read_string(&out->detail)) {
            return false;
        }
        if (!valid_bulk_stream_type(type) || !valid_bulk_channel_state(state)) {
            return false;
        }
        out->stream_type = static_cast<BulkStreamType>(type);
        out->state = static_cast<BulkChannelState>(state);
        return true;
    });
}

}  // namespace yunlink
