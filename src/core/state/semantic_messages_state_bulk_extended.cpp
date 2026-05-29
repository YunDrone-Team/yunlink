/**
 * @file src/core/state/semantic_messages_state_bulk_extended.cpp
 * @brief Extended state, event and bulk payload codec implementations.
 */

#include "state_codec_common.hpp"

namespace yunlink {

using namespace state_codec_detail;

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
