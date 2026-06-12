/**
 * @file src/core/state/semantic_messages_state_snapshots_extended.cpp
 * @brief Compound snapshot payload codec implementations.
 */

#include "state_codec_common.hpp"

namespace yunlink {

using namespace state_codec_detail;

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

ByteBuffer encode_payload(const SunrayRuntimeDiagnosticSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_string(payload.agent_key);
        writer.write_u32(payload.stale_timeout_ms);
        write_topic_diagnostic(writer, payload.external_odom);
        write_topic_diagnostic(writer, payload.odom_state);
        write_topic_diagnostic(writer, payload.local_odom);
        write_topic_diagnostic(writer, payload.global_odom);
        write_topic_diagnostic(writer, payload.uav_control_cmd);
        write_topic_diagnostic(writer, payload.uav_control_state);
        write_topic_diagnostic(writer, payload.px4_state);
        writer.write_string(payload.worst_level);
        writer.write_string(payload.summary);
    });
}

bool decode_payload(const ByteBuffer& bytes, SunrayRuntimeDiagnosticSnapshot* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, SunrayRuntimeDiagnosticSnapshot* out) {
            return read_header(reader, &out->header) && reader.read_string(&out->agent_key) &&
                   reader.read_u32(&out->stale_timeout_ms) &&
                   read_topic_diagnostic(reader, &out->external_odom) &&
                   read_topic_diagnostic(reader, &out->odom_state) &&
                   read_topic_diagnostic(reader, &out->local_odom) &&
                   read_topic_diagnostic(reader, &out->global_odom) &&
                   read_topic_diagnostic(reader, &out->uav_control_cmd) &&
                   read_topic_diagnostic(reader, &out->uav_control_state) &&
                   read_topic_diagnostic(reader, &out->px4_state) &&
                   reader.read_string(&out->worst_level) && reader.read_string(&out->summary);
        });
}

ByteBuffer encode_payload(const CommandExecutionStatusSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_header(writer, payload.header);
        writer.write_string(payload.agent_name);
        writer.write_u8(payload.agent_id);
        writer.write_u64(payload.session_id);
        writer.write_u64(payload.command_message_id);
        writer.write_u64(payload.command_correlation_id);
        writer.write_u16(static_cast<uint16_t>(payload.command_kind));
        writer.write_u8(payload.execution_state);
        writer.write_u8(payload.progress_percent);
        writer.write_bool(payload.active);
        writer.write_bool(payload.terminal);
        writer.write_bool(payload.success);
        writer.write_u16(payload.result_code);
        writer.write_string(payload.detail);
        writer.write_u8(payload.control_state);
        writer.write_u8(payload.px4_landed_state);
        writer.write_bool(payload.ready_for_takeoff);
        writer.write_bool(payload.ready_for_land);
        writer.write_string(payload.busy_reason);
    });
}

bool decode_payload(const ByteBuffer& bytes, CommandExecutionStatusSnapshot* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, CommandExecutionStatusSnapshot* out) {
            uint16_t command_kind = 0;
            if (!read_header(reader, &out->header) || !reader.read_string(&out->agent_name) ||
                !reader.read_u8(&out->agent_id) || !reader.read_u64(&out->session_id) ||
                !reader.read_u64(&out->command_message_id) ||
                !reader.read_u64(&out->command_correlation_id) || !reader.read_u16(&command_kind) ||
                !reader.read_u8(&out->execution_state) || !reader.read_u8(&out->progress_percent) ||
                !reader.read_bool(&out->active) || !reader.read_bool(&out->terminal) ||
                !reader.read_bool(&out->success) || !reader.read_u16(&out->result_code) ||
                !reader.read_string(&out->detail) || !reader.read_u8(&out->control_state) ||
                !reader.read_u8(&out->px4_landed_state) ||
                !reader.read_bool(&out->ready_for_takeoff) ||
                !reader.read_bool(&out->ready_for_land) || !reader.read_string(&out->busy_reason)) {
                return false;
            }

            if (!valid_command_kind(command_kind) ||
                !valid_command_execution_state(out->execution_state)) {
                return false;
            }
            out->command_kind = static_cast<CommandKind>(command_kind);
            return true;
        });
}

}  // namespace yunlink
