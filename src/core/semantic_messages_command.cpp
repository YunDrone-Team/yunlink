/**
 * @file src/core/semantic_messages_command.cpp
 * @brief Command 与 CommandResult payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

namespace {

bool valid_command_kind(uint16_t value) {
    return value <= static_cast<uint16_t>(CommandKind::kFormationTask);
}

bool valid_command_phase(uint8_t value) {
    return value >= static_cast<uint8_t>(CommandPhase::kReceived) &&
           value <= static_cast<uint8_t>(CommandPhase::kExpired);
}

}  // namespace

ByteBuffer encode_payload(const TakeoffCommand& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_float(payload.relative_height_m);
        writer.write_float(payload.max_velocity_mps);
    });
}

bool decode_payload(const ByteBuffer& bytes, TakeoffCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TakeoffCommand* out) {
        return reader.read_float(&out->relative_height_m) &&
               reader.read_float(&out->max_velocity_mps);
    });
}

ByteBuffer encode_payload(const LandCommand& payload) {
    return build_payload(
        [&](BufferWriter& writer) { writer.write_float(payload.max_velocity_mps); });
}

bool decode_payload(const ByteBuffer& bytes, LandCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, LandCommand* out) {
        return reader.read_float(&out->max_velocity_mps);
    });
}

ByteBuffer encode_payload(const ReturnCommand& payload) {
    return build_payload(
        [&](BufferWriter& writer) { writer.write_float(payload.loiter_before_return_s); });
}

bool decode_payload(const ByteBuffer& bytes, ReturnCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ReturnCommand* out) {
        return reader.read_float(&out->loiter_before_return_s);
    });
}

ByteBuffer encode_payload(const GotoCommand& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_float(payload.x_m);
        writer.write_float(payload.y_m);
        writer.write_float(payload.z_m);
        writer.write_float(payload.yaw_rad);
    });
}

bool decode_payload(const ByteBuffer& bytes, GotoCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, GotoCommand* out) {
        return reader.read_float(&out->x_m) && reader.read_float(&out->y_m) &&
               reader.read_float(&out->z_m) && reader.read_float(&out->yaw_rad);
    });
}

ByteBuffer encode_payload(const VelocitySetpointCommand& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_float(payload.vx_mps);
        writer.write_float(payload.vy_mps);
        writer.write_float(payload.vz_mps);
        writer.write_float(payload.yaw_rate_radps);
        writer.write_bool(payload.body_frame);
    });
}

bool decode_payload(const ByteBuffer& bytes, VelocitySetpointCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, VelocitySetpointCommand* out) {
        return reader.read_float(&out->vx_mps) && reader.read_float(&out->vy_mps) &&
               reader.read_float(&out->vz_mps) && reader.read_float(&out->yaw_rate_radps) &&
               reader.read_bool(&out->body_frame);
    });
}

ByteBuffer encode_payload(const TrajectoryChunkCommand& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u32(payload.chunk_index);
        writer.write_bool(payload.final_chunk);
        writer.write_u16(static_cast<uint16_t>(payload.points.size()));
        for (const auto& point : payload.points) {
            writer.write_float(point.x_m);
            writer.write_float(point.y_m);
            writer.write_float(point.z_m);
            writer.write_float(point.vx_mps);
            writer.write_float(point.vy_mps);
            writer.write_float(point.vz_mps);
            writer.write_float(point.yaw_rad);
            writer.write_u32(point.dt_ms);
        }
    });
}

bool decode_payload(const ByteBuffer& bytes, TrajectoryChunkCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TrajectoryChunkCommand* out) {
        uint16_t count = 0;
        if (!reader.read_u32(&out->chunk_index) || !reader.read_bool(&out->final_chunk) ||
            !reader.read_u16(&count)) {
            return false;
        }

        out->points.clear();
        for (uint16_t i = 0; i < count; ++i) {
            TrajectoryPoint point;
            if (!reader.read_float(&point.x_m) || !reader.read_float(&point.y_m) ||
                !reader.read_float(&point.z_m) || !reader.read_float(&point.vx_mps) ||
                !reader.read_float(&point.vy_mps) || !reader.read_float(&point.vz_mps) ||
                !reader.read_float(&point.yaw_rad) || !reader.read_u32(&point.dt_ms)) {
                return false;
            }
            out->points.push_back(point);
        }
        return true;
    });
}

ByteBuffer encode_payload(const FormationTaskCommand& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u32(payload.group_id);
        writer.write_u8(payload.formation_shape);
        writer.write_float(payload.spacing_m);
        writer.write_string(payload.label);
    });
}

bool decode_payload(const ByteBuffer& bytes, FormationTaskCommand* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FormationTaskCommand* out) {
        return reader.read_u32(&out->group_id) && reader.read_u8(&out->formation_shape) &&
               reader.read_float(&out->spacing_m) && reader.read_string(&out->label);
    });
}

ByteBuffer encode_payload(const CommandResult& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u16(static_cast<uint16_t>(payload.command_kind));
        writer.write_u8(static_cast<uint8_t>(payload.phase));
        writer.write_u16(payload.result_code);
        writer.write_u8(payload.progress_percent);
        writer.write_string(payload.detail);
    });
}

bool decode_payload(const ByteBuffer& bytes, CommandResult* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, CommandResult* out) {
        uint16_t kind = 0;
        uint8_t phase = 0;
        if (!reader.read_u16(&kind) || !reader.read_u8(&phase) ||
            !reader.read_u16(&out->result_code) || !reader.read_u8(&out->progress_percent) ||
            !reader.read_string(&out->detail)) {
            return false;
        }
        if (!valid_command_kind(kind) || !valid_command_phase(phase)) {
            return false;
        }
        out->command_kind = static_cast<CommandKind>(kind);
        out->phase = static_cast<CommandPhase>(phase);
        return true;
    });
}

}  // namespace yunlink
