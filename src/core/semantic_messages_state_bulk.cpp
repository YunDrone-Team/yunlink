/**
 * @file src/core/semantic_messages_state_bulk.cpp
 * @brief State 与 Bulk payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace sunraycom {

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
        out->kind = static_cast<VehicleEventKind>(kind);
        return true;
    });
}

ByteBuffer encode_payload(const BulkChannelDescriptor& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(static_cast<uint8_t>(payload.stream_type));
        writer.write_string(payload.uri);
        writer.write_u32(payload.mtu_bytes);
        writer.write_bool(payload.reliable);
    });
}

bool decode_payload(const ByteBuffer& bytes, BulkChannelDescriptor* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, BulkChannelDescriptor* out) {
        uint8_t type = 0;
        if (!reader.read_u8(&type) || !reader.read_string(&out->uri) ||
            !reader.read_u32(&out->mtu_bytes) || !reader.read_bool(&out->reliable)) {
            return false;
        }
        out->stream_type = static_cast<BulkStreamType>(type);
        return true;
    });
}

}  // namespace sunraycom
