/**
 * @file tests/core/test_protocol_corruption.cpp
 * @brief SecureEnvelope corruption and variable-header validation tests.
 */

#include <iostream>

#include "yunlink/core/protocol_codec.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace {

yunlink::SecureEnvelope make_envelope() {
    const yunlink::EndpointIdentity source{
        yunlink::AgentType::kGroundStation,
        17,
        yunlink::EndpointRole::kController,
    };
    auto target = yunlink::TargetSelector::for_group(yunlink::AgentType::kUav, 42);
    target.target_ids = {42, 43};

    yunlink::GotoCommand payload{};
    payload.x_m = 1.0F;
    payload.y_m = 2.0F;
    payload.z_m = 3.0F;
    payload.yaw_rad = 0.25F;

    auto envelope = yunlink::make_typed_envelope(
        source, target, 7001, 8001, yunlink::QosClass::kReliableOrdered, payload, 1000);
    envelope.message_id = 9001;
    envelope.correlation_id = 9001;
    envelope.security.key_epoch = 4;
    envelope.security.auth_tag = {0xAA, 0x55, 0x10, 0x20};
    return envelope;
}

bool decode_with_code(const yunlink::ProtocolCodec& codec,
                      const yunlink::ByteBuffer& bytes,
                      yunlink::ErrorCode expected) {
    const auto result = codec.decode(bytes.data(), bytes.size());
    return result.code == expected;
}

}  // namespace

int main() {
    const yunlink::ProtocolCodec codec;
    const auto bytes = codec.encode(make_envelope(), true);
    if (bytes.empty()) {
        std::cerr << "fixture encode failed\n";
        return 1;
    }

    auto bad_magic = bytes;
    bad_magic[0] = 0x00;
    if (!decode_with_code(codec, bad_magic, yunlink::ErrorCode::kInvalidHeader)) {
        std::cerr << "bad magic did not return invalid header\n";
        return 2;
    }

    auto bad_header_len_small = bytes;
    bad_header_len_small[8] = 75;
    bad_header_len_small[9] = 0;
    if (!decode_with_code(codec, bad_header_len_small, yunlink::ErrorCode::kDecodeError)) {
        std::cerr << "short header_len did not return decode error\n";
        return 3;
    }

    auto bad_payload_len = bytes;
    bad_payload_len[10] = 0xFF;
    bad_payload_len[11] = 0xFF;
    bad_payload_len[12] = 0x00;
    bad_payload_len[13] = 0x00;
    if (!decode_with_code(codec, bad_payload_len, yunlink::ErrorCode::kDecodeError)) {
        std::cerr << "oversized payload_len did not return decode error\n";
        return 4;
    }

    auto bad_target_count = bytes;
    bad_target_count[68] = 0x01;
    bad_target_count[69] = 0x00;
    if (!decode_with_code(codec, bad_target_count, yunlink::ErrorCode::kDecodeError)) {
        std::cerr << "target_count/header_len mismatch did not return decode error\n";
        return 5;
    }

    auto bad_auth_tag_len = bytes;
    bad_auth_tag_len[74] = 0x05;
    bad_auth_tag_len[75] = 0x00;
    if (!decode_with_code(codec, bad_auth_tag_len, yunlink::ErrorCode::kDecodeError)) {
        std::cerr << "auth_tag_len/header_len mismatch did not return decode error\n";
        return 6;
    }

    auto bad_checksum = bytes;
    bad_checksum[bad_checksum.size() - 1] ^= 0xFF;
    if (!decode_with_code(codec, bad_checksum, yunlink::ErrorCode::kChecksumMismatch)) {
        std::cerr << "checksum corruption did not return checksum mismatch\n";
        return 7;
    }

    return 0;
}
