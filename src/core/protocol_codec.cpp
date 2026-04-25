/**
 * @file src/core/protocol_codec.cpp
 * @brief yunlink source file.
 */

#include "yunlink/core/protocol_codec.hpp"

#include <chrono>

namespace yunlink {

namespace {

uint64_t now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

void append_u16(ByteBuffer& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFFU));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
}

void append_u32(ByteBuffer& out, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

void append_u64(ByteBuffer& out, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

uint16_t read_u16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t read_u32(const uint8_t* data) {
    uint32_t out = 0;
    for (int i = 0; i < 4; ++i) {
        out |= static_cast<uint32_t>(data[i]) << (i * 8);
    }
    return out;
}

uint64_t read_u64(const uint8_t* data) {
    uint64_t out = 0;
    for (int i = 0; i < 8; ++i) {
        out |= static_cast<uint64_t>(data[i]) << (i * 8);
    }
    return out;
}

}  // namespace

bool ProtocolCodec::has_magic(const uint8_t* data, size_t len) {
    return data != nullptr && len >= 4 && data[0] == kMagic0 && data[1] == kMagic1 &&
           data[2] == kMagic2 && data[3] == kMagic3;
}

uint32_t ProtocolCodec::checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += static_cast<uint32_t>(data[i]);
    }
    return sum;
}

ByteBuffer ProtocolCodec::encode(const SecureEnvelope& envelope, bool auto_fill_header) const {
    SecureEnvelope working = envelope;
    if (auto_fill_header) {
        working.header_len = static_cast<uint16_t>(
            kFixedHeaderSize + working.target.target_ids.size() * sizeof(uint32_t) +
            working.security.auth_tag.size());
        working.payload_len = static_cast<uint32_t>(working.payload.size());
        if (working.created_at_ms == 0) {
            working.created_at_ms = now_millis();
        }
        if (working.message_id == 0) {
            working.message_id = working.created_at_ms;
        }
    }

    if (working.header_len < kFixedHeaderSize) {
        return {};
    }

    ByteBuffer out;
    out.reserve(static_cast<size_t>(working.header_len) + working.payload.size() + kTrailerSize);

    out.push_back(kMagic0);
    out.push_back(kMagic1);
    out.push_back(kMagic2);
    out.push_back(kMagic3);
    out.push_back(working.protocol_major);
    out.push_back(working.header_version);
    append_u16(out, working.flags);
    append_u16(out, working.header_len);
    append_u32(out, working.payload_len);
    out.push_back(static_cast<uint8_t>(working.qos_class));
    out.push_back(static_cast<uint8_t>(working.message_family));
    append_u16(out, working.message_type);
    append_u16(out, working.schema_version);
    append_u64(out, working.session_id);
    append_u64(out, working.message_id);
    append_u64(out, working.correlation_id);
    append_u64(out, working.created_at_ms);
    append_u32(out, working.ttl_ms);
    out.push_back(static_cast<uint8_t>(working.source.agent_type));
    append_u32(out, working.source.agent_id);
    out.push_back(static_cast<uint8_t>(working.source.role));
    out.push_back(static_cast<uint8_t>(working.target.scope));
    out.push_back(static_cast<uint8_t>(working.target.target_type));
    append_u32(out, working.target.group_id);
    append_u16(out, static_cast<uint16_t>(working.target.target_ids.size()));
    append_u32(out, working.security.key_epoch);
    append_u16(out, static_cast<uint16_t>(working.security.auth_tag.size()));

    for (uint32_t id : working.target.target_ids) {
        append_u32(out, id);
    }
    out.insert(out.end(), working.security.auth_tag.begin(), working.security.auth_tag.end());
    out.insert(out.end(), working.payload.begin(), working.payload.end());

    const uint32_t wire_checksum = checksum(out.data(), out.size());
    append_u32(out, wire_checksum);
    return out;
}

DecodeResult ProtocolCodec::decode(const uint8_t* data, size_t len, uint64_t now_ms) const {
    DecodeResult result;
    if (data == nullptr || len < kMinEnvelopeSize) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }
    if (!has_magic(data, len)) {
        result.code = ErrorCode::kInvalidHeader;
        return result;
    }

    const uint16_t header_len = read_u16(data + 8);
    const uint32_t payload_len = read_u32(data + 10);
    if (header_len < kFixedHeaderSize) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }

    const size_t total_len = static_cast<size_t>(header_len) + payload_len + kTrailerSize;
    if (len < total_len) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }

    const uint16_t target_count = read_u16(data + 68);
    const uint16_t auth_tag_len = read_u16(data + 74);
    const size_t variable_header_len =
        static_cast<size_t>(target_count) * sizeof(uint32_t) + static_cast<size_t>(auth_tag_len);
    if (header_len != kFixedHeaderSize + variable_header_len) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }

    const uint32_t wire_checksum = read_u32(data + total_len - kTrailerSize);
    const uint32_t calc_checksum = checksum(data, total_len - kTrailerSize);
    if (wire_checksum != calc_checksum) {
        result.code = ErrorCode::kChecksumMismatch;
        result.consumed = total_len;
        return result;
    }

    SecureEnvelope envelope;
    envelope.protocol_major = data[4];
    envelope.header_version = data[5];
    envelope.flags = read_u16(data + 6);
    envelope.header_len = header_len;
    envelope.payload_len = payload_len;
    envelope.qos_class = static_cast<QosClass>(data[14]);
    envelope.message_family = static_cast<MessageFamily>(data[15]);
    envelope.message_type = read_u16(data + 16);
    envelope.schema_version = read_u16(data + 18);
    envelope.session_id = read_u64(data + 20);
    envelope.message_id = read_u64(data + 28);
    envelope.correlation_id = read_u64(data + 36);
    envelope.created_at_ms = read_u64(data + 44);
    envelope.ttl_ms = read_u32(data + 52);
    envelope.source.agent_type = static_cast<AgentType>(data[56]);
    envelope.source.agent_id = read_u32(data + 57);
    envelope.source.role = static_cast<EndpointRole>(data[61]);
    envelope.target.scope = static_cast<TargetScope>(data[62]);
    envelope.target.target_type = static_cast<AgentType>(data[63]);
    envelope.target.group_id = read_u32(data + 64);
    envelope.security.key_epoch = read_u32(data + 70);

    size_t cursor = kFixedHeaderSize;
    for (uint16_t i = 0; i < target_count; ++i) {
        envelope.target.target_ids.push_back(read_u32(data + cursor));
        cursor += sizeof(uint32_t);
    }
    envelope.security.auth_tag.assign(data + cursor, data + cursor + auth_tag_len);
    cursor += auth_tag_len;
    envelope.payload.assign(data + cursor, data + cursor + payload_len);
    envelope.checksum = wire_checksum;

    if (envelope.ttl_ms > 0 && now_ms > 0 && envelope.created_at_ms + envelope.ttl_ms < now_ms) {
        result.code = ErrorCode::kTimeout;
        result.consumed = total_len;
        return result;
    }

    result.code = ErrorCode::kOk;
    result.consumed = total_len;
    result.envelope = std::move(envelope);
    return result;
}

}  // namespace yunlink
