/**
 * @file src/runtime/core/security.cpp
 * @brief Runtime envelope validation and security helpers.
 */

#include "security.hpp"

namespace yunlink {
namespace {

void fnv_mix_u8(uint64_t* hash, uint8_t value) {
    *hash ^= value;
    *hash *= 1099511628211ULL;
}

void fnv_mix_u16(uint64_t* hash, uint16_t value) {
    fnv_mix_u8(hash, static_cast<uint8_t>(value & 0xFFU));
    fnv_mix_u8(hash, static_cast<uint8_t>((value >> 8) & 0xFFU));
}

void fnv_mix_u32(uint64_t* hash, uint32_t value) {
    for (int i = 0; i < 4; ++i) {
        fnv_mix_u8(hash, static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

void fnv_mix_u64(uint64_t* hash, uint64_t value) {
    for (int i = 0; i < 8; ++i) {
        fnv_mix_u8(hash, static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
    }
}

void fnv_mix_bytes(uint64_t* hash, const ByteBuffer& bytes) {
    for (uint8_t byte : bytes) {
        fnv_mix_u8(hash, byte);
    }
}

void fnv_mix_string(uint64_t* hash, const std::string& value) {
    for (char ch : value) {
        fnv_mix_u8(hash, static_cast<uint8_t>(ch));
    }
}

}  // namespace

bool runtime_envelope_expired(const SecureEnvelope& envelope, uint64_t now_ms) {
    if (envelope.ttl_ms == 0 || envelope.created_at_ms == 0 || now_ms <= envelope.created_at_ms) {
        return false;
    }
    return now_ms - envelope.created_at_ms > envelope.ttl_ms;
}

bool runtime_protocol_version_mismatch(const SecureEnvelope& envelope) {
    return envelope.protocol_major != 1 || envelope.header_version != 1;
}

bool runtime_schema_version_mismatch(const SecureEnvelope& envelope) {
    return envelope.schema_version != kCurrentSchemaVersion;
}

bool runtime_security_tags_required(const RuntimeConfig& config) {
    return config.security_tags_required ||
           (config.capability_flags & kCapabilitySecurityTags) != 0;
}

bool runtime_security_tags_enabled(const RuntimeConfig& config) {
    return config.security_tags_enabled || runtime_security_tags_required(config);
}

ByteBuffer make_runtime_auth_tag(const RuntimeConfig& config, const SecureEnvelope& envelope) {
    uint64_t hash = 1469598103934665603ULL;
    fnv_mix_string(&hash, config.shared_secret);
    fnv_mix_u32(&hash, envelope.security.key_epoch);
    fnv_mix_u8(&hash, envelope.protocol_major);
    fnv_mix_u8(&hash, envelope.header_version);
    fnv_mix_u16(&hash, envelope.flags);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.qos_class));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.message_family));
    fnv_mix_u16(&hash, envelope.message_type);
    fnv_mix_u16(&hash, envelope.schema_version);
    fnv_mix_u64(&hash, envelope.session_id);
    fnv_mix_u64(&hash, envelope.message_id);
    fnv_mix_u64(&hash, envelope.correlation_id);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.source.agent_type));
    fnv_mix_u32(&hash, envelope.source.agent_id);
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.source.role));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.target.scope));
    fnv_mix_u8(&hash, static_cast<uint8_t>(envelope.target.target_type));
    fnv_mix_u32(&hash, envelope.target.group_id);
    for (uint32_t id : envelope.target.target_ids) {
        fnv_mix_u32(&hash, id);
    }
    fnv_mix_u64(&hash, envelope.created_at_ms);
    fnv_mix_u32(&hash, envelope.ttl_ms);
    fnv_mix_bytes(&hash, envelope.payload);

    ByteBuffer tag(8);
    for (int i = 0; i < 8; ++i) {
        tag[static_cast<size_t>(i)] = static_cast<uint8_t>((hash >> (i * 8)) & 0xFFU);
    }
    return tag;
}

void apply_runtime_security_tag(const RuntimeConfig& config, SecureEnvelope* envelope) {
    if (!runtime_security_tags_enabled(config) || envelope == nullptr) {
        return;
    }
    envelope->security.key_epoch = config.security_key_epoch;
    envelope->security.auth_tag = make_runtime_auth_tag(config, *envelope);
}

std::string runtime_security_replay_key(const SecureEnvelope& envelope) {
    return std::to_string(envelope.security.key_epoch) + ":" +
           std::to_string(static_cast<uint8_t>(envelope.source.agent_type)) + ":" +
           std::to_string(envelope.source.agent_id) + ":" + std::to_string(envelope.message_id);
}

}  // namespace yunlink
