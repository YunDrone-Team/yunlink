/**
 * @file src/core/semantic_messages_session_authority.cpp
 * @brief Session 与 Authority payload 编解码。
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

ByteBuffer encode_payload(const SessionHello& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.node_name);
        writer.write_u32(payload.capability_flags);
    });
}

bool decode_payload(const ByteBuffer& bytes, SessionHello* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, SessionHello* out) {
        return reader.read_string(&out->node_name) && reader.read_u32(&out->capability_flags);
    });
}

ByteBuffer encode_payload(const SessionAuthenticate& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_string(payload.shared_secret); });
}

bool decode_payload(const ByteBuffer& bytes, SessionAuthenticate* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, SessionAuthenticate* out) {
        return reader.read_string(&out->shared_secret);
    });
}

ByteBuffer encode_payload(const SessionCapabilities& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u32(payload.capability_flags); });
}

bool decode_payload(const ByteBuffer& bytes, SessionCapabilities* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, SessionCapabilities* out) {
        return reader.read_u32(&out->capability_flags);
    });
}

ByteBuffer encode_payload(const SessionReady& payload) {
    return build_payload(
        [&](BufferWriter& writer) { writer.write_u8(payload.accepted_protocol_major); });
}

bool decode_payload(const ByteBuffer& bytes, SessionReady* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, SessionReady* out) {
        return reader.read_u8(&out->accepted_protocol_major);
    });
}

ByteBuffer encode_payload(const AuthorityRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(static_cast<uint8_t>(payload.action));
        writer.write_u8(static_cast<uint8_t>(payload.source));
        writer.write_u32(payload.lease_ttl_ms);
        writer.write_bool(payload.allow_preempt);
    });
}

bool decode_payload(const ByteBuffer& bytes, AuthorityRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, AuthorityRequest* out) {
        uint8_t action = 0;
        uint8_t source = 0;
        if (!reader.read_u8(&action) || !reader.read_u8(&source) ||
            !reader.read_u32(&out->lease_ttl_ms) || !reader.read_bool(&out->allow_preempt)) {
            return false;
        }
        out->action = static_cast<AuthorityAction>(action);
        out->source = static_cast<ControlSource>(source);
        return true;
    });
}

ByteBuffer encode_payload(const AuthorityStatus& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_u8(static_cast<uint8_t>(payload.state));
        writer.write_u64(payload.session_id);
        writer.write_u32(payload.lease_ttl_ms);
        writer.write_u16(payload.reason_code);
        writer.write_string(payload.detail);
    });
}

bool decode_payload(const ByteBuffer& bytes, AuthorityStatus* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, AuthorityStatus* out) {
        uint8_t state = 0;
        if (!reader.read_u8(&state) || !reader.read_u64(&out->session_id) ||
            !reader.read_u32(&out->lease_ttl_ms) || !reader.read_u16(&out->reason_code) ||
            !reader.read_string(&out->detail)) {
            return false;
        }
        out->state = static_cast<AuthorityState>(state);
        return true;
    });
}

}  // namespace yunlink
