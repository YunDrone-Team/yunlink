/**
 * @file tests/test_protocol.cpp
 * @brief yunlink source file.
 */

#include <iostream>

#include "yunlink/core/protocol_codec.hpp"
#include "yunlink/core/semantic_messages.hpp"

int main() {
    yunlink::ProtocolCodec codec;
    const yunlink::EndpointIdentity source{
        yunlink::AgentType::kGroundStation,
        7,
        yunlink::EndpointRole::kController,
    };
    const yunlink::TargetSelector target =
        yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 42);

    yunlink::GotoCommand payload{};
    payload.x_m = 1.25F;
    payload.y_m = -2.5F;
    payload.z_m = 8.0F;
    payload.yaw_rad = 0.33F;

    auto envelope = yunlink::make_typed_envelope(
        source, target, 1001, 9001, yunlink::QosClass::kReliableOrdered, payload, 25);
    envelope.security.key_epoch = 3;
    envelope.security.auth_tag = {0xAA, 0x55, 0x10, 0x20};

    const auto bytes = codec.encode(envelope, true);
    if (bytes.empty()) {
        std::cerr << "encode failed\n";
        return 1;
    }

    const auto dr = codec.decode(bytes.data(), bytes.size(), envelope.created_at_ms);
    if (!dr.ok()) {
        std::cerr << "decode failed\n";
        return 2;
    }

    if (dr.envelope.protocol_major != envelope.protocol_major ||
        dr.envelope.message_id != envelope.message_id ||
        dr.envelope.correlation_id != envelope.correlation_id ||
        dr.envelope.source.agent_id != source.agent_id ||
        dr.envelope.target.group_id != target.group_id ||
        dr.envelope.target.scope != yunlink::TargetScope::kGroup ||
        dr.envelope.message_family != yunlink::MessageFamily::kCommand ||
        dr.envelope.message_type != yunlink::MessageTraits<yunlink::GotoCommand>::kMessageType) {
        std::cerr << "roundtrip mismatch\n";
        return 3;
    }

    yunlink::GotoCommand decoded{};
    if (!yunlink::decode_typed_payload(dr.envelope.payload, &decoded) ||
        decoded.x_m != payload.x_m || decoded.z_m != payload.z_m ||
        decoded.yaw_rad != payload.yaw_rad) {
        std::cerr << "typed payload mismatch\n";
        return 4;
    }

    yunlink::AuthorityStatus authority_status{};
    authority_status.state = yunlink::AuthorityState::kController;
    authority_status.session_id = 0x12345678ABCDEF01ULL;
    authority_status.lease_ttl_ms = 2000;
    authority_status.reason_code = 9;
    authority_status.detail = "lease";

    const auto authority_bytes = yunlink::encode_payload(authority_status);
    yunlink::AuthorityStatus authority_decoded{};
    if (!yunlink::decode_typed_payload(authority_bytes, &authority_decoded)) {
        std::cerr << "authority status decode failed\n";
        return 5;
    }
    if (authority_decoded.session_id != authority_status.session_id) {
        std::cerr << "authority status session id truncated\n";
        return 6;
    }

    auto corrupted = bytes;
    corrupted[0] = 0x00;
    const auto bad = codec.decode(corrupted.data(), corrupted.size(), envelope.created_at_ms);
    if (bad.code != yunlink::ErrorCode::kInvalidHeader) {
        std::cerr << "invalid header not detected\n";
        return 7;
    }

    const auto expired =
        codec.decode(bytes.data(), bytes.size(), envelope.created_at_ms + envelope.ttl_ms + 1);
    if (expired.code != yunlink::ErrorCode::kTimeout) {
        std::cerr << "ttl expiration not detected\n";
        return 8;
    }

    yunlink::LocalOdomSnapshot local_odom{};
    local_odom.pose.position_m = {1.0F, -2.0F, 3.0F};
    local_odom.pose.orientation = {0.1F, 0.2F, 0.3F, 0.9F};
    local_odom.twist.linear_mps = {0.4F, 0.5F, 0.6F};

    const auto local_odom_bytes = yunlink::encode_payload(local_odom);
    yunlink::LocalOdomSnapshot local_odom_decoded{};
    if (!yunlink::decode_typed_payload(local_odom_bytes, &local_odom_decoded) ||
        local_odom_decoded.pose.position_m.y != local_odom.pose.position_m.y ||
        local_odom_decoded.pose.orientation.w != local_odom.pose.orientation.w ||
        local_odom_decoded.twist.linear_mps.z != local_odom.twist.linear_mps.z) {
        std::cerr << "local odom roundtrip failed\n";
        return 9;
    }

    yunlink::UavControlStateSnapshot control_state{};
    control_state.controller_types = 3;
    control_state.takeoff_relative_height_m = 2.5;
    control_state.takeoff_max_velocity_mps = 1.5;
    control_state.land_type = 2;
    control_state.land_max_velocity_mps = 0.8;
    control_state.home_point_m = {4.0F, 5.0F, 6.0F};
    control_state.control_state = 7;
    control_state.last_cmd.control_cmd = 9;
    control_state.last_cmd.cmd_source = 1;
    control_state.odometry_lost = false;
    control_state.odometry_valid = true;
    control_state.self_odom.pose.position_m.z = 1.75F;

    const auto control_state_bytes = yunlink::encode_payload(control_state);
    yunlink::UavControlStateSnapshot control_state_decoded{};
    if (!yunlink::decode_typed_payload(control_state_bytes, &control_state_decoded) ||
        control_state_decoded.controller_types != control_state.controller_types ||
        control_state_decoded.home_point_m.z != control_state.home_point_m.z ||
        !control_state_decoded.odometry_valid ||
        control_state_decoded.self_odom.pose.position_m.z !=
            control_state.self_odom.pose.position_m.z) {
        std::cerr << "uav control state roundtrip failed\n";
        return 10;
    }

    yunlink::OdomStateSnapshot odom_state{};
    odom_state.external_source = 2;
    odom_state.subtopic_name_external_odom = "/uav1/sunray/localization/external_odom";
    odom_state.odometry_valid = true;
    odom_state.odometry_update_hz = 47.5F;
    odom_state.subtopic_name_external_relocalization = "/uav1/sunray/localization/relocalization";
    odom_state.pubtopic_name_local_odom = "/uav1/sunray/localization/local_odom";
    odom_state.pubtopic_name_global_odom = "/uav1/sunray/localization/global_odom";
    odom_state.world_frame_name = "world";
    odom_state.global_frame_name = "map";
    odom_state.local_frame_name = "odom";
    odom_state.base_frame_name = "base_link";

    const auto odom_state_bytes = yunlink::encode_payload(odom_state);
    yunlink::OdomStateSnapshot odom_state_decoded{};
    if (!yunlink::decode_typed_payload(odom_state_bytes, &odom_state_decoded) ||
        odom_state_decoded.external_source != odom_state.external_source ||
        odom_state_decoded.pubtopic_name_local_odom != odom_state.pubtopic_name_local_odom ||
        odom_state_decoded.base_frame_name != odom_state.base_frame_name) {
        std::cerr << "odom state roundtrip failed\n";
        return 11;
    }

    return 0;
}
