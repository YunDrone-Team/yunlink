#include "yunlink/core/core_messages_v2.hpp"
#include "core_messages_v2_internal.hpp"

namespace yunlink::v2 {
Bytes encode(const EntityDirectory& value) {
    Writer writer;
    writer.text(value.endpoint_uid);
    writer.text(value.revision);
    writer.list<EntityDescriptor>(value.entities,
                                  [&](const auto& entity) { write_entity(writer, entity); });
    return writer.take();
}

Bytes encode(const AttachmentRequest& value) {
    Writer writer;
    writer.text(value.expected_revision);
    writer.list<std::string>(value.entity_uids, [&](const auto& uid) { writer.text(uid); });
    return writer.take();
}

Bytes encode(const AttachmentResponse& value) {
    Writer writer;
    writer.u8(value.success ? 1 : 0);
    writer.text(value.revision);
    writer.list<std::string>(value.attached_entity_uids,
                             [&](const auto& uid) { writer.text(uid); });
    writer.text(value.message);
    return writer.take();
}

Bytes encode(const AuthorityRequest& value) {
    Writer writer;
    writer.text(value.authority_scope);
    writer.u32(value.lease_ttl_ms);
    writer.u8(value.allow_preempt ? 1 : 0);
    return writer.take();
}

Bytes encode(const AuthorityStatus& value) {
    Writer writer;
    writer.text(value.authority_scope);
    writer.text(value.state);
    writer.u32(value.lease_ttl_ms);
    writer.u16(value.reason_code);
    writer.text(value.detail);
    return writer.take();
}

Bytes encode(const StreamCatalog& value) {
    Writer writer;
    writer.text(value.revision);
    writer.list<StreamDescriptor>(value.streams, [&](const auto& stream) {
        writer.text(stream.stream_uid);
        write_type(writer, stream.type);
        writer.text(stream.encoding);
        writer.map(stream.metadata);
    });
    return writer.take();
}

Bytes encode(const StreamSubscription& value) {
    Writer writer;
    writer.text(value.stream_uid);
    writer.f32(value.max_rate_hz);
    writer.u32(value.max_payload_bytes);
    return writer.take();
}

Bytes encode(const StreamSample& value) {
    Writer writer;
    writer.text(value.stream_uid);
    writer.text(value.encoding);
    writer.map(value.metadata);
    writer.u64(value.source_timestamp_ns);
    writer.u64(value.sequence);
    writer.bytes(value.data);
    return writer.take();
}

Bytes encode(const ActionUpdate& value) {
    Writer writer;
    writer.u8(static_cast<uint8_t>(value.phase));
    writer.u16(value.result_code);
    writer.u8(value.progress_percent);
    writer.text(value.detail);
    return writer.take();
}

Bytes encode(const BulkDescriptor& value) {
    Writer writer;
    writer.text(value.media_type);
    writer.text(value.encoding);
    writer.map(value.metadata);
    writer.u64(value.total_bytes);
    return writer.take();
}

Bytes encode(const LogListResponse& value) {
    Writer writer;
    writer.u8(value.success ? 1 : 0);
    writer.text(value.message);
    writer.list<LogSummary>(value.logs, [&](const auto& log) { write_log_summary(writer, log); });
    return writer.take();
}

Bytes encode(const LogReadRequest& value) {
    Writer writer;
    writer.text(value.log_uid);
    writer.u64(value.cursor);
    writer.u32(value.max_bytes);
    return writer.take();
}

Bytes encode(const LogReadResponse& value) {
    Writer writer;
    writer.u8(value.success ? 1 : 0);
    writer.text(value.message);
    writer.text(value.log_uid);
    writer.text(value.chunk);
    writer.u64(value.next_cursor);
    writer.u8(value.truncated ? 1 : 0);
    writer.u8(value.eof ? 1 : 0);
    return writer.take();
}
bool decode(const Bytes& bytes, EntityDirectory* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->endpoint_uid) && valid_uid(out->endpoint_uid) &&
               reader.text(&out->revision) &&
               reader.list<EntityDescriptor>(
                   &out->entities, [&](auto* entity) { return read_entity(reader, entity); });
    });
}

bool decode(const Bytes& bytes, AttachmentRequest* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->expected_revision) &&
               reader.list<std::string>(&out->entity_uids, [&](auto* uid) {
                   return reader.text(uid) && valid_uid(*uid);
               });
    });
}

bool decode(const Bytes& bytes, AttachmentResponse* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        uint8_t success = 0;
        return reader.u8(&success) && success <= 1 && (out->success = success != 0, true) &&
               reader.text(&out->revision) &&
               reader.list<std::string>(
                   &out->attached_entity_uids,
                   [&](auto* uid) { return reader.text(uid) && valid_uid(*uid); }) &&
               reader.text(&out->message);
    });
}

bool decode(const Bytes& bytes, AuthorityRequest* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        uint8_t preempt = 0;
        return reader.text(&out->authority_scope) && reader.u32(&out->lease_ttl_ms) &&
               reader.u8(&preempt) && preempt <= 1 && (out->allow_preempt = preempt != 0, true);
    });
}

bool decode(const Bytes& bytes, AuthorityStatus* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->authority_scope) && reader.text(&out->state) &&
               reader.u32(&out->lease_ttl_ms) && reader.u16(&out->reason_code) &&
               reader.text(&out->detail);
    });
}

bool decode(const Bytes& bytes, StreamCatalog* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->revision) &&
               reader.list<StreamDescriptor>(&out->streams, [&](auto* stream) {
                   return reader.text(&stream->stream_uid) && valid_uid(stream->stream_uid) &&
                          read_type(reader, &stream->type) && reader.text(&stream->encoding) &&
                          reader.string_map(&stream->metadata);
               });
    });
}

bool decode(const Bytes& bytes, StreamSubscription* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->stream_uid) && valid_uid(out->stream_uid) &&
               reader.f32(&out->max_rate_hz) && reader.u32(&out->max_payload_bytes);
    });
}

bool decode(const Bytes& bytes, StreamSample* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->stream_uid) && valid_uid(out->stream_uid) &&
               reader.text(&out->encoding) && reader.string_map(&out->metadata) &&
               reader.u64(&out->source_timestamp_ns) && reader.u64(&out->sequence) &&
               reader.bytes(&out->data);
    });
}

bool decode(const Bytes& bytes, ActionUpdate* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        uint8_t phase = 0;
        return reader.u8(&phase) && phase >= static_cast<uint8_t>(ActionPhase::kReceived) &&
               phase <= static_cast<uint8_t>(ActionPhase::kExpired) &&
               (out->phase = static_cast<ActionPhase>(phase), true) &&
               reader.u16(&out->result_code) && reader.u8(&out->progress_percent) &&
               out->progress_percent <= 100 && reader.text(&out->detail);
    });
}

bool decode(const Bytes& bytes, BulkDescriptor* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->media_type) && reader.text(&out->encoding) &&
               reader.string_map(&out->metadata) && reader.u64(&out->total_bytes);
    });
}

bool decode(const Bytes& bytes, LogListResponse* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        uint8_t success = 0;
        return reader.u8(&success) && success <= 1 && (out->success = success != 0, true) &&
               reader.text(&out->message) && reader.list<LogSummary>(&out->logs, [&](auto* log) {
                   return read_log_summary(reader, log);
               });
    });
}

bool decode(const Bytes& bytes, LogReadRequest* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        return reader.text(&out->log_uid) && valid_uid(out->log_uid) && reader.u64(&out->cursor) &&
               reader.u32(&out->max_bytes) && out->max_bytes > 0;
    });
}

bool decode(const Bytes& bytes, LogReadResponse* value) {
    return decode_all(bytes, value, [](Reader& reader, auto* out) {
        uint8_t success = 0;
        uint8_t truncated = 0;
        uint8_t eof = 0;
        return reader.u8(&success) && success <= 1 && (out->success = success != 0, true) &&
               reader.text(&out->message) && reader.text(&out->log_uid) &&
               valid_uid(out->log_uid) && reader.text(&out->chunk) &&
               reader.u64(&out->next_cursor) && reader.u8(&truncated) && truncated <= 1 &&
               (out->truncated = truncated != 0, true) && reader.u8(&eof) && eof <= 1 &&
               (out->eof = eof != 0, true);
    });
}
}  // namespace yunlink::v2
