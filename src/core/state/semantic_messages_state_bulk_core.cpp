/**
 * @file src/core/state/semantic_messages_state_bulk_core.cpp
 * @brief Core state and status payload codec implementations.
 */

#include "state_codec_common.hpp"

namespace yunlink {

using namespace state_codec_detail;

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
        write_header(writer, payload.header);
        writer.write_bool(payload.connected);
        writer.write_bool(payload.rc_available);
        writer.write_bool(payload.armed);
        writer.write_string(payload.flight_mode);
        writer.write_u8(payload.system_status);
        writer.write_u8(payload.landed_state);
        writer.write_float(payload.battery_voltage_v);
        writer.write_float(payload.battery_current_a);
        writer.write_float(payload.battery_percentage);
        writer.write_u16(payload.fcu_load);
        write_pose(writer, payload.external_pose);
        write_twist(writer, payload.external_velocity);
        write_pose(writer, payload.local_pose);
        write_twist(writer, payload.local_velocity);
        writer.write_u8(payload.setpoint_coordinate_frame);
        writer.write_u16(payload.setpoint_local_type_mask);
        write_vec3(writer, payload.pos_setpoint_m);
        write_vec3(writer, payload.vel_setpoint_mps);
        write_vec3(writer, payload.acc_setpoint_mps2);
        writer.write_float(payload.yaw_setpoint_rad);
        writer.write_float(payload.yaw_rate_setpoint_radps);
        writer.write_u16(payload.setpoint_att_type_mask);
        write_quat(writer, payload.orientation_setpoint);
        write_vec3(writer, payload.body_rate_setpoint_radps);
        writer.write_float(payload.thrust_setpoint);
        writer.write_u8(payload.satellites);
        writer.write_i8(payload.gps_status);
        writer.write_double(payload.latitude_deg);
        writer.write_double(payload.longitude_deg);
        writer.write_double(payload.altitude_m);
        writer.write_double(payload.latitude_raw_deg);
        writer.write_double(payload.longitude_raw_deg);
        writer.write_double(payload.altitude_amsl_m);
    });
}

bool decode_payload(const ByteBuffer& bytes, Px4StateSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, Px4StateSnapshot* out) {
        return read_header(reader, &out->header) && reader.read_bool(&out->connected) &&
               reader.read_bool(&out->rc_available) && reader.read_bool(&out->armed) &&
               reader.read_string(&out->flight_mode) && reader.read_u8(&out->system_status) &&
               reader.read_u8(&out->landed_state) && reader.read_float(&out->battery_voltage_v) &&
               reader.read_float(&out->battery_current_a) &&
               reader.read_float(&out->battery_percentage) && reader.read_u16(&out->fcu_load) &&
               read_pose(reader, &out->external_pose) &&
               read_twist(reader, &out->external_velocity) && read_pose(reader, &out->local_pose) &&
               read_twist(reader, &out->local_velocity) &&
               reader.read_u8(&out->setpoint_coordinate_frame) &&
               reader.read_u16(&out->setpoint_local_type_mask) &&
               read_vec3(reader, &out->pos_setpoint_m) &&
               read_vec3(reader, &out->vel_setpoint_mps) &&
               read_vec3(reader, &out->acc_setpoint_mps2) &&
               reader.read_float(&out->yaw_setpoint_rad) &&
               reader.read_float(&out->yaw_rate_setpoint_radps) &&
               reader.read_u16(&out->setpoint_att_type_mask) &&
               read_quat(reader, &out->orientation_setpoint) &&
               read_vec3(reader, &out->body_rate_setpoint_radps) &&
               reader.read_float(&out->thrust_setpoint) && reader.read_u8(&out->satellites) &&
               reader.read_i8(&out->gps_status) && reader.read_double(&out->latitude_deg) &&
               reader.read_double(&out->longitude_deg) && reader.read_double(&out->altitude_m) &&
               reader.read_double(&out->latitude_raw_deg) &&
               reader.read_double(&out->longitude_raw_deg) &&
               reader.read_double(&out->altitude_amsl_m);
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

}  // namespace yunlink
