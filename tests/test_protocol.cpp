/**
 * @file tests/test_protocol.cpp
 * @brief sunray_communication_lib source file.
 */

#include <iostream>

#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/core/semantic_messages.hpp"

int main() {
    sunraycom::ProtocolCodec codec;
    const sunraycom::EndpointIdentity source{
        sunraycom::AgentType::kGroundStation,
        7,
        sunraycom::EndpointRole::kController,
    };
    const sunraycom::TargetSelector target =
        sunraycom::TargetSelector::for_group(sunraycom::AgentType::kUav, 42);

    sunraycom::GotoCommand payload{};
    payload.x_m = 1.25F;
    payload.y_m = -2.5F;
    payload.z_m = 8.0F;
    payload.yaw_rad = 0.33F;

    auto envelope = sunraycom::make_typed_envelope(
        source, target, 1001, 9001, sunraycom::QosClass::kReliableOrdered, payload, 25);
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
        dr.envelope.target.scope != sunraycom::TargetScope::kGroup ||
        dr.envelope.message_family != sunraycom::MessageFamily::kCommand ||
        dr.envelope.message_type !=
            sunraycom::MessageTraits<sunraycom::GotoCommand>::kMessageType) {
        std::cerr << "roundtrip mismatch\n";
        return 3;
    }

    sunraycom::GotoCommand decoded{};
    if (!sunraycom::decode_typed_payload(dr.envelope.payload, &decoded) ||
        decoded.x_m != payload.x_m || decoded.z_m != payload.z_m ||
        decoded.yaw_rad != payload.yaw_rad) {
        std::cerr << "typed payload mismatch\n";
        return 4;
    }

    sunraycom::AuthorityStatus authority_status{};
    authority_status.state = sunraycom::AuthorityState::kController;
    authority_status.session_id = 0x12345678ABCDEF01ULL;
    authority_status.lease_ttl_ms = 2000;
    authority_status.reason_code = 9;
    authority_status.detail = "lease";

    const auto authority_bytes = sunraycom::encode_payload(authority_status);
    sunraycom::AuthorityStatus authority_decoded{};
    if (!sunraycom::decode_typed_payload(authority_bytes, &authority_decoded)) {
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
    if (bad.code != sunraycom::ErrorCode::kInvalidHeader) {
        std::cerr << "invalid header not detected\n";
        return 7;
    }

    const auto expired =
        codec.decode(bytes.data(), bytes.size(), envelope.created_at_ms + envelope.ttl_ms + 1);
    if (expired.code != sunraycom::ErrorCode::kTimeout) {
        std::cerr << "ttl expiration not detected\n";
        return 8;
    }

    return 0;
}
