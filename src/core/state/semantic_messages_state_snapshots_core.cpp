/**
 * @file src/core/state/semantic_messages_state_snapshots_core.cpp
 * @brief Basic snapshot payload codec implementations.
 */

#include "state_codec_common.hpp"

namespace yunlink {

using namespace state_codec_detail;

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

ByteBuffer encode_payload(const UavControlCmdSnapshot& payload) {
    return build_payload([&](BufferWriter& writer) { write_uav_control_cmd(writer, payload); });
}

bool decode_payload(const ByteBuffer& bytes, UavControlCmdSnapshot* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, UavControlCmdSnapshot* out) {
        return read_uav_control_cmd(reader, out);
    });
}

}  // namespace yunlink
