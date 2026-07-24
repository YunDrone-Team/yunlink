/** @file src/core/wire_v2_codec.cpp */

#include "yunlink/core/wire_v2_codec.hpp"

#include <chrono>
#include <limits>

namespace yunlink::v2 {
namespace {

uint64_t now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

void append_u16(Bytes& out, uint16_t value) {
    out.push_back(static_cast<uint8_t>(value));
    out.push_back(static_cast<uint8_t>(value >> 8U));
}

void append_u32(Bytes& out, uint32_t value) {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void append_u64(Bytes& out, uint64_t value) {
    for (unsigned shift = 0; shift < 64; shift += 8) {
        out.push_back(static_cast<uint8_t>(value >> shift));
    }
}

void append_text(Bytes& out, const std::string& value) {
    out.insert(out.end(), value.begin(), value.end());
}

uint16_t read_u16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | static_cast<uint16_t>(data[1] << 8U);
}

uint32_t read_u32(const uint8_t* data) {
    uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        value |= static_cast<uint32_t>(data[shift / 8]) << shift;
    }
    return value;
}

uint64_t read_u64(const uint8_t* data) {
    uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) {
        value |= static_cast<uint64_t>(data[shift / 8]) << shift;
    }
    return value;
}

bool valid_family(uint8_t value) {
    return value >= static_cast<uint8_t>(MessageFamily::kSession) &&
           value <= static_cast<uint8_t>(MessageFamily::kBulk);
}

bool valid_qos(uint8_t value) {
    return value >= static_cast<uint8_t>(QosClass::kReliableOrdered) &&
           value <= static_cast<uint8_t>(QosClass::kBulk);
}

bool valid_scope(uint8_t value) {
    return value >= static_cast<uint8_t>(TargetScope::kEndpoint) &&
           value <= static_cast<uint8_t>(TargetScope::kBroadcast);
}

}  // namespace

bool WireCodec::has_magic(const uint8_t* data, size_t len) {
    return data != nullptr && len >= 4 && data[0] == kMagic0 && data[1] == kMagic1 &&
           data[2] == kMagic2 && data[3] == kMagic3;
}

uint32_t WireCodec::checksum(const uint8_t* data, size_t len) {
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < len; ++i) {
        hash ^= data[i];
        hash *= 16777619U;
    }
    return hash;
}

Bytes WireCodec::encode(const Envelope& input, bool auto_fill_header) const {
    Envelope value = input;
    if (value.protocol_major != kProtocolMajor || value.header_version != kHeaderVersion ||
        value.schema_version != kSchemaVersion || !valid_uid(value.source.endpoint_uid) ||
        (!value.source.entity_uid.empty() && !valid_uid(value.source.entity_uid)) ||
        !valid_type_ref(value.type) || value.payload.size() > kMaxPayloadBytes ||
        value.target.uids.size() > std::numeric_limits<uint16_t>::max()) {
        return {};
    }
    if (value.target.scope == TargetScope::kBroadcast) {
        if (!value.target.uids.empty()) {
            return {};
        }
    } else if (value.target.uids.empty()) {
        return {};
    }
    size_t targets_bytes = 0;
    for (const auto& uid : value.target.uids) {
        if (!valid_uid(uid)) {
            return {};
        }
        targets_bytes += sizeof(uint16_t) + uid.size();
    }
    const size_t variable_size = value.source.endpoint_uid.size() + value.source.entity_uid.size() +
                                 targets_bytes + value.security.auth_tag.size() +
                                 value.type.profile_id.size() + value.type.type_name.size();
    if (variable_size > std::numeric_limits<uint16_t>::max() - kFixedHeaderSize ||
        value.security.auth_tag.size() > std::numeric_limits<uint16_t>::max()) {
        return {};
    }
    if (auto_fill_header) {
        value.header_len = static_cast<uint16_t>(kFixedHeaderSize + variable_size);
        value.payload_len = static_cast<uint32_t>(value.payload.size());
        if (value.created_at_ms == 0) {
            value.created_at_ms = now_millis();
        }
        if (value.message_id == 0) {
            value.message_id = value.created_at_ms;
        }
    }
    if (value.header_len != kFixedHeaderSize + variable_size ||
        value.payload_len != value.payload.size()) {
        return {};
    }

    Bytes out;
    out.reserve(value.header_len + value.payload.size() + kTrailerSize);
    out.insert(out.end(), {kMagic0, kMagic1, kMagic2, kMagic3});
    out.push_back(value.protocol_major);
    out.push_back(value.header_version);
    append_u16(out, value.flags);
    append_u16(out, value.header_len);
    append_u32(out, value.payload_len);
    out.push_back(static_cast<uint8_t>(value.qos_class));
    out.push_back(static_cast<uint8_t>(value.family));
    out.push_back(value.operation);
    out.push_back(0);
    append_u16(out, value.schema_version);
    append_u64(out, value.session_id);
    append_u64(out, value.message_id);
    append_u64(out, value.correlation_id);
    append_u64(out, value.created_at_ms);
    append_u32(out, value.ttl_ms);
    append_u32(out, value.security.key_epoch);
    append_u16(out, static_cast<uint16_t>(value.source.endpoint_uid.size()));
    append_u16(out, static_cast<uint16_t>(value.source.entity_uid.size()));
    append_u16(out, static_cast<uint16_t>(value.target.uids.size()));
    append_u16(out, static_cast<uint16_t>(value.security.auth_tag.size()));
    append_u16(out, static_cast<uint16_t>(value.type.profile_id.size()));
    append_u16(out, static_cast<uint16_t>(value.type.type_name.size()));
    out.push_back(static_cast<uint8_t>(value.target.scope));
    out.push_back(0);
    append_u16(out, value.type.major);
    append_u16(out, value.type.minor);
    append_u16(out, 0);
    append_text(out, value.source.endpoint_uid);
    append_text(out, value.source.entity_uid);
    for (const auto& uid : value.target.uids) {
        append_u16(out, static_cast<uint16_t>(uid.size()));
        append_text(out, uid);
    }
    out.insert(out.end(), value.security.auth_tag.begin(), value.security.auth_tag.end());
    append_text(out, value.type.profile_id);
    append_text(out, value.type.type_name);
    out.insert(out.end(), value.payload.begin(), value.payload.end());
    append_u32(out, checksum(out.data(), out.size()));
    return out;
}

DecodeResult WireCodec::decode(const uint8_t* data, size_t len, uint64_t now_ms) const {
    DecodeResult result;
    if (data == nullptr || len < kMinEnvelopeSize) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }
    if (!has_magic(data, len)) {
        result.code = ErrorCode::kInvalidHeader;
        return result;
    }
    if (data[4] != kProtocolMajor || data[5] != kHeaderVersion ||
        read_u16(data + 18) != kSchemaVersion) {
        result.code = ErrorCode::kProtocolMismatch;
        return result;
    }
    const uint16_t header_len = read_u16(data + 8);
    const uint32_t payload_len = read_u32(data + 10);
    if (header_len < kFixedHeaderSize || payload_len > kMaxPayloadBytes) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }
    const size_t total_len = static_cast<size_t>(header_len) + payload_len + kTrailerSize;
    if (len < total_len) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }
    if (!valid_qos(data[14]) || !valid_family(data[15]) || !valid_scope(data[72])) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    const uint16_t endpoint_len = read_u16(data + 60);
    const uint16_t entity_len = read_u16(data + 62);
    const uint16_t target_count = read_u16(data + 64);
    const uint16_t auth_len = read_u16(data + 66);
    const uint16_t profile_len = read_u16(data + 68);
    const uint16_t type_len = read_u16(data + 70);
    size_t cursor = kFixedHeaderSize;
    auto read_text = [&](size_t text_len, std::string* out) {
        if (text_len > static_cast<size_t>(header_len) - cursor) {
            return false;
        }
        out->assign(reinterpret_cast<const char*>(data + cursor), text_len);
        cursor += text_len;
        return true;
    };

    Envelope value;
    value.protocol_major = data[4];
    value.header_version = data[5];
    value.flags = read_u16(data + 6);
    value.header_len = header_len;
    value.payload_len = payload_len;
    value.qos_class = static_cast<QosClass>(data[14]);
    value.family = static_cast<MessageFamily>(data[15]);
    value.operation = data[16];
    value.schema_version = read_u16(data + 18);
    value.session_id = read_u64(data + 20);
    value.message_id = read_u64(data + 28);
    value.correlation_id = read_u64(data + 36);
    value.created_at_ms = read_u64(data + 44);
    value.ttl_ms = read_u32(data + 52);
    value.security.key_epoch = read_u32(data + 56);
    value.target.scope = static_cast<TargetScope>(data[72]);
    value.type.major = read_u16(data + 74);
    value.type.minor = read_u16(data + 76);
    if (!read_text(endpoint_len, &value.source.endpoint_uid) ||
        !read_text(entity_len, &value.source.entity_uid)) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    for (uint16_t i = 0; i < target_count; ++i) {
        if (cursor + sizeof(uint16_t) > header_len) {
            result.code = ErrorCode::kDecodeError;
            result.consumed = total_len;
            return result;
        }
        const uint16_t uid_len = read_u16(data + cursor);
        cursor += sizeof(uint16_t);
        std::string uid;
        if (!read_text(uid_len, &uid)) {
            result.code = ErrorCode::kDecodeError;
            result.consumed = total_len;
            return result;
        }
        value.target.uids.push_back(std::move(uid));
    }
    if (auth_len > static_cast<size_t>(header_len) - cursor) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    value.security.auth_tag.assign(data + cursor, data + cursor + auth_len);
    cursor += auth_len;
    if (!read_text(profile_len, &value.type.profile_id) ||
        !read_text(type_len, &value.type.type_name) || cursor != header_len) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    if (!valid_uid(value.source.endpoint_uid) ||
        (!value.source.entity_uid.empty() && !valid_uid(value.source.entity_uid)) ||
        !valid_type_ref(value.type)) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    if ((value.target.scope == TargetScope::kBroadcast) != value.target.uids.empty()) {
        result.code = ErrorCode::kDecodeError;
        result.consumed = total_len;
        return result;
    }
    for (const auto& uid : value.target.uids) {
        if (!valid_uid(uid)) {
            result.code = ErrorCode::kDecodeError;
            result.consumed = total_len;
            return result;
        }
    }
    const uint32_t wire_checksum = read_u32(data + total_len - kTrailerSize);
    if (wire_checksum != checksum(data, total_len - kTrailerSize)) {
        result.code = ErrorCode::kChecksumMismatch;
        result.consumed = total_len;
        return result;
    }
    if (value.ttl_ms > 0 && now_ms > 0 &&
        (value.created_at_ms > std::numeric_limits<uint64_t>::max() - value.ttl_ms ||
         value.created_at_ms + value.ttl_ms < now_ms)) {
        result.code = ErrorCode::kTimeout;
        result.consumed = total_len;
        return result;
    }
    value.payload.assign(data + header_len, data + header_len + payload_len);
    value.checksum = wire_checksum;
    result.envelope = std::move(value);
    result.consumed = total_len;
    return result;
}

}  // namespace yunlink::v2
