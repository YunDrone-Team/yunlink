/**
 * @file src/core/semantic_messages_topic.cpp
 * @brief Topic discovery, subscription and sample payload codecs.
 */

#include "semantic_codec_io.hpp"

namespace yunlink {
namespace {

constexpr uint32_t kMaxTopicBlobBytes = 4U * 1024U * 1024U;

void write_long_string(BufferWriter& writer, const std::string& value) {
    if (value.size() > kMaxTopicBlobBytes) {
        writer.invalidate();
        return;
    }
    writer.write_u32(static_cast<uint32_t>(value.size()));
    writer.data.insert(writer.data.end(), value.begin(), value.end());
}

bool read_long_string(BufferReader& reader, std::string* out) {
    uint32_t size = 0;
    if (out == nullptr || !reader.read_u32(&size) || size > kMaxTopicBlobBytes ||
        reader.cursor + size > reader.data.size()) {
        return false;
    }
    out->assign(reinterpret_cast<const char*>(reader.data.data() + reader.cursor), size);
    reader.cursor += size;
    return true;
}

void write_blob(BufferWriter& writer, const ByteBuffer& value) {
    if (value.size() > kMaxTopicBlobBytes) {
        writer.invalidate();
        return;
    }
    writer.write_u32(static_cast<uint32_t>(value.size()));
    writer.data.insert(writer.data.end(), value.begin(), value.end());
}

bool read_blob(BufferReader& reader, ByteBuffer* out) {
    uint32_t size = 0;
    if (out == nullptr || !reader.read_u32(&size) || size > kMaxTopicBlobBytes ||
        reader.cursor + size > reader.data.size()) {
        return false;
    }
    out->assign(reader.data.begin() + static_cast<std::ptrdiff_t>(reader.cursor),
                reader.data.begin() + static_cast<std::ptrdiff_t>(reader.cursor + size));
    reader.cursor += size;
    return true;
}

void write_topic_descriptor(BufferWriter& writer, const TopicDescriptor& value) {
    writer.write_string(value.name);
    writer.write_string(value.type_name);
    writer.write_u32(value.publisher_count);
}

bool read_topic_descriptor(BufferReader& reader, TopicDescriptor* out) {
    return out != nullptr && reader.read_string(&out->name) &&
           reader.read_string(&out->type_name) && reader.read_u32(&out->publisher_count);
}

}  // namespace

ByteBuffer encode_payload(const TopicListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u8(payload.reserved); });
}

bool decode_payload(const ByteBuffer& bytes, TopicListRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TopicListRequest* out) {
        return reader.read_u8(&out->reserved);
    });
}

ByteBuffer encode_payload(const TopicListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.revision);
        if (payload.topics.size() > UINT16_MAX) {
            writer.invalidate();
            return;
        }
        writer.write_u16(static_cast<uint16_t>(payload.topics.size()));
        for (const auto& topic : payload.topics) {
            write_topic_descriptor(writer, topic);
        }
    });
}

bool decode_payload(const ByteBuffer& bytes, TopicListResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TopicListResponse* out) {
        uint16_t count = 0;
        if (!reader.read_bool(&out->success) || !reader.read_string(&out->message) ||
            !reader.read_string(&out->revision) || !reader.read_u16(&count)) {
            return false;
        }
        out->topics.clear();
        out->topics.reserve(count);
        for (uint16_t index = 0; index < count; ++index) {
            TopicDescriptor topic;
            if (!read_topic_descriptor(reader, &topic)) {
                return false;
            }
            out->topics.push_back(std::move(topic));
        }
        return true;
    });
}

ByteBuffer encode_payload(const TopicSubscriptionRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.topic_name);
        writer.write_bool(payload.subscribe);
        writer.write_float(payload.max_rate_hz);
        writer.write_u32(payload.max_payload_bytes);
    });
}

bool decode_payload(const ByteBuffer& bytes, TopicSubscriptionRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TopicSubscriptionRequest* out) {
        return reader.read_string(&out->topic_name) && reader.read_bool(&out->subscribe) &&
               reader.read_float(&out->max_rate_hz) && reader.read_u32(&out->max_payload_bytes);
    });
}

ByteBuffer encode_payload(const TopicSubscriptionResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.topic_name);
        writer.write_bool(payload.subscribed);
        writer.write_string(payload.type_name);
        writer.write_float(payload.max_rate_hz);
        writer.write_u32(payload.max_payload_bytes);
    });
}

bool decode_payload(const ByteBuffer& bytes, TopicSubscriptionResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TopicSubscriptionResponse* out) {
        return reader.read_bool(&out->success) && reader.read_string(&out->message) &&
               reader.read_string(&out->topic_name) && reader.read_bool(&out->subscribed) &&
               reader.read_string(&out->type_name) && reader.read_float(&out->max_rate_hz) &&
               reader.read_u32(&out->max_payload_bytes);
    });
}

ByteBuffer encode_payload(const TopicSample& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.topic_name);
        writer.write_string(payload.type_name);
        writer.write_string(payload.type_hash);
        writer.write_string(payload.encoding);
        write_long_string(writer, payload.message_definition);
        writer.write_u64(payload.receive_time_ns);
        writer.write_u64(payload.sequence);
        writer.write_bool(payload.metadata_included);
        write_blob(writer, payload.data);
    });
}

bool decode_payload(const ByteBuffer& bytes, TopicSample* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, TopicSample* out) {
        return reader.read_string(&out->topic_name) && reader.read_string(&out->type_name) &&
               reader.read_string(&out->type_hash) && reader.read_string(&out->encoding) &&
               read_long_string(reader, &out->message_definition) &&
               reader.read_u64(&out->receive_time_ns) && reader.read_u64(&out->sequence) &&
               reader.read_bool(&out->metadata_included) && read_blob(reader, &out->data);
    });
}

}  // namespace yunlink
