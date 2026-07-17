/**
 * @file src/core/semantic_messages_system_service.cpp
 * @brief System service payload codec implementation.
 */

#include "semantic_codec_io.hpp"

namespace yunlink {

namespace {

void write_string_vector(BufferWriter& writer, const std::vector<std::string>& values) {
    writer.write_u16(static_cast<uint16_t>(values.size()));
    for (const auto& value : values) {
        writer.write_string(value);
    }
}

bool read_string_vector(BufferReader& reader, std::vector<std::string>* out) {
    uint16_t count = 0;
    if (!reader.read_u16(&count) || out == nullptr) {
        return false;
    }
    out->clear();
    out->reserve(count);
    for (uint16_t i = 0; i < count; ++i) {
        std::string value;
        if (!reader.read_string(&value)) {
            return false;
        }
        out->push_back(std::move(value));
    }
    return true;
}

void write_runtime_log_summary(BufferWriter& writer, const RuntimeLogSummary& value) {
    writer.write_string(value.runtime_id);
    writer.write_string(value.feature_name);
    writer.write_string(value.title);
    writer.write_string(value.state);
    writer.write_u64(value.started_at_ns);
    writer.write_u64(value.finished_at_ns);
    writer.write_bool(value.has_exit_code);
    writer.write_u32(static_cast<uint32_t>(value.exit_code));
    writer.write_string(value.message);
}

bool read_runtime_log_summary(BufferReader& reader, RuntimeLogSummary* out) {
    uint32_t exit_code = 0;
    return out != nullptr && reader.read_string(&out->runtime_id) &&
           reader.read_string(&out->feature_name) && reader.read_string(&out->title) &&
           reader.read_string(&out->state) && reader.read_u64(&out->started_at_ns) &&
           reader.read_u64(&out->finished_at_ns) && reader.read_bool(&out->has_exit_code) &&
           reader.read_u32(&exit_code) && reader.read_string(&out->message) &&
           ((out->exit_code = static_cast<int32_t>(exit_code)), true);
}

void write_runtime_log_summaries(BufferWriter& writer,
                                 const std::vector<RuntimeLogSummary>& values) {
    writer.write_u16(static_cast<uint16_t>(values.size()));
    for (const auto& value : values) {
        write_runtime_log_summary(writer, value);
    }
}

bool read_runtime_log_summaries(BufferReader& reader, std::vector<RuntimeLogSummary>* out) {
    uint16_t count = 0;
    if (out == nullptr || !reader.read_u16(&count)) {
        return false;
    }
    out->clear();
    out->reserve(count);
    for (uint16_t index = 0; index < count; ++index) {
        RuntimeLogSummary value;
        if (!read_runtime_log_summary(reader, &value)) {
            return false;
        }
        out->push_back(std::move(value));
    }
    return true;
}

}  // namespace

ByteBuffer encode_payload(const FeatureListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u8(payload.reserved); });
}

bool decode_payload(const ByteBuffer& bytes, FeatureListRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureListRequest* out) {
        return reader.read_u8(&out->reserved);
    });
}

ByteBuffer encode_payload(const FeatureListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        write_string_vector(writer, payload.feature_names);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureListResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureListResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               read_string_vector(reader, &out->feature_names);
    });
}

ByteBuffer encode_payload(const FeatureGetRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_string(payload.feature_name); });
}

bool decode_payload(const ByteBuffer& bytes, FeatureGetRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureGetRequest* out) {
        return reader.read_string(&out->feature_name);
    });
}

ByteBuffer encode_payload(const FeatureGetResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.name);
        writer.write_string(payload.title);
        writer.write_string(payload.group);
        writer.write_bool(payload.running);
        writer.write_string(payload.description);
        writer.write_bool(payload.auto_start);
        write_string_vector(writer, payload.depends_on);
        write_string_vector(writer, payload.start_preview_units);
        write_string_vector(writer, payload.start_preview_commands);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureGetResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureGetResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               reader.read_string(&out->name) && reader.read_string(&out->title) &&
               reader.read_string(&out->group) && reader.read_bool(&out->running) &&
               reader.read_string(&out->description) && reader.read_bool(&out->auto_start) &&
               read_string_vector(reader, &out->depends_on) &&
               read_string_vector(reader, &out->start_preview_units) &&
               read_string_vector(reader, &out->start_preview_commands);
    });
}

ByteBuffer encode_payload(const FeatureStartRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.feature_name);
        write_string_vector(writer, payload.override_args);
        writer.write_bool(payload.restart_if_running);
        writer.write_bool(payload.start_with_terminal);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureStartRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureStartRequest* out) {
        return reader.read_string(&out->feature_name) &&
               read_string_vector(reader, &out->override_args) &&
               reader.read_bool(&out->restart_if_running) &&
               reader.read_bool(&out->start_with_terminal);
    });
}

ByteBuffer encode_payload(const FeatureStartResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.feature_name);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureStartResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureStartResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               reader.read_string(&out->feature_name);
    });
}

ByteBuffer encode_payload(const FeatureStopRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.feature_name);
        writer.write_bool(payload.force);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureStopRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureStopRequest* out) {
        return reader.read_string(&out->feature_name) && reader.read_bool(&out->force);
    });
}

ByteBuffer encode_payload(const FeatureStopResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.feature_name);
    });
}

bool decode_payload(const ByteBuffer& bytes, FeatureStopResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, FeatureStopResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               reader.read_string(&out->feature_name);
    });
}

ByteBuffer encode_payload(const RuntimeLogListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u8(payload.reserved); });
}

bool decode_payload(const ByteBuffer& bytes, RuntimeLogListRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, RuntimeLogListRequest* out) {
        return reader.read_u8(&out->reserved);
    });
}

ByteBuffer encode_payload(const RuntimeLogListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        write_runtime_log_summaries(writer, payload.runtimes);
    });
}

bool decode_payload(const ByteBuffer& bytes, RuntimeLogListResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, RuntimeLogListResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               read_runtime_log_summaries(reader, &out->runtimes);
    });
}

ByteBuffer encode_payload(const RuntimeLogReadRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.runtime_id);
        writer.write_u64(payload.cursor);
        writer.write_u32(payload.max_bytes);
    });
}

bool decode_payload(const ByteBuffer& bytes, RuntimeLogReadRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, RuntimeLogReadRequest* out) {
        return reader.read_string(&out->runtime_id) && reader.read_u64(&out->cursor) &&
               reader.read_u32(&out->max_bytes);
    });
}

ByteBuffer encode_payload(const RuntimeLogReadResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.runtime_id);
        writer.write_string(payload.chunk);
        writer.write_u64(payload.next_cursor);
        writer.write_bool(payload.truncated);
        writer.write_bool(payload.eof);
    });
}

bool decode_payload(const ByteBuffer& bytes, RuntimeLogReadResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, RuntimeLogReadResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               reader.read_string(&out->runtime_id) && reader.read_string(&out->chunk) &&
               reader.read_u64(&out->next_cursor) && reader.read_bool(&out->truncated) &&
               reader.read_bool(&out->eof);
    });
}

}  // namespace yunlink
