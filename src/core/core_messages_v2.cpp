#include "yunlink/core/core_messages_v2.hpp"

#include <cstring>
#include <limits>

namespace yunlink::v2 {
namespace {

class Writer {
  public:
    void u8(uint8_t value) {
        data_.push_back(value);
    }
    void u16(uint16_t value) {
        data_.push_back(static_cast<uint8_t>(value));
        data_.push_back(static_cast<uint8_t>(value >> 8U));
    }
    void u32(uint32_t value) {
        for (unsigned shift = 0; shift < 32; shift += 8) {
            data_.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
    void u64(uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            data_.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
    void f32(float value) {
        uint32_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "float must be IEEE-754 binary32");
        std::memcpy(&bits, &value, sizeof(bits));
        u32(bits);
    }
    bool text(const std::string& value) {
        if (value.size() > UINT16_MAX) {
            valid_ = false;
            return false;
        }
        u16(static_cast<uint16_t>(value.size()));
        data_.insert(data_.end(), value.begin(), value.end());
        return true;
    }
    template <typename T, typename Callback>
    bool list(const std::vector<T>& values, Callback callback) {
        if (values.size() > UINT16_MAX) {
            valid_ = false;
            return false;
        }
        u16(static_cast<uint16_t>(values.size()));
        for (const auto& value : values) {
            callback(value);
        }
        return valid_;
    }
    bool map(const std::map<std::string, std::string>& values) {
        if (values.size() > UINT16_MAX) {
            valid_ = false;
            return false;
        }
        u16(static_cast<uint16_t>(values.size()));
        for (const auto& entry : values) {
            text(entry.first);
            text(entry.second);
        }
        return valid_;
    }
    Bytes take() {
        return valid_ ? std::move(data_) : Bytes{};
    }

  private:
    Bytes data_;
    bool valid_ = true;
};

class Reader {
  public:
    explicit Reader(const Bytes& bytes) : bytes_(bytes) {}
    bool u8(uint8_t* value) {
        if (value == nullptr || cursor_ + 1 > bytes_.size()) {
            return false;
        }
        *value = bytes_[cursor_++];
        return true;
    }
    bool u16(uint16_t* value) {
        if (value == nullptr || cursor_ + 2 > bytes_.size()) {
            return false;
        }
        *value = static_cast<uint16_t>(bytes_[cursor_]) |
                 static_cast<uint16_t>(bytes_[cursor_ + 1] << 8U);
        cursor_ += 2;
        return true;
    }
    bool u32(uint32_t* value) {
        if (value == nullptr || cursor_ + 4 > bytes_.size()) {
            return false;
        }
        *value = 0;
        for (unsigned shift = 0; shift < 32; shift += 8) {
            *value |= static_cast<uint32_t>(bytes_[cursor_++]) << shift;
        }
        return true;
    }
    bool u64(uint64_t* value) {
        if (value == nullptr || cursor_ + 8 > bytes_.size()) {
            return false;
        }
        *value = 0;
        for (unsigned shift = 0; shift < 64; shift += 8) {
            *value |= static_cast<uint64_t>(bytes_[cursor_++]) << shift;
        }
        return true;
    }
    bool f32(float* value) {
        uint32_t bits = 0;
        if (value == nullptr || !u32(&bits)) {
            return false;
        }
        std::memcpy(value, &bits, sizeof(bits));
        return true;
    }
    bool text(std::string* value) {
        uint16_t size = 0;
        if (value == nullptr || !u16(&size) || cursor_ + size > bytes_.size()) {
            return false;
        }
        value->assign(reinterpret_cast<const char*>(bytes_.data() + cursor_), size);
        cursor_ += size;
        return true;
    }
    template <typename T, typename Callback> bool list(std::vector<T>* values, Callback callback) {
        uint16_t count = 0;
        if (values == nullptr || !u16(&count)) {
            return false;
        }
        values->clear();
        values->reserve(count);
        for (uint16_t i = 0; i < count; ++i) {
            T value;
            if (!callback(&value)) {
                return false;
            }
            values->push_back(std::move(value));
        }
        return true;
    }
    bool string_map(std::map<std::string, std::string>* values) {
        uint16_t count = 0;
        if (values == nullptr || !u16(&count)) {
            return false;
        }
        values->clear();
        for (uint16_t i = 0; i < count; ++i) {
            std::string key;
            std::string value;
            if (!text(&key) || !text(&value) || !values->emplace(key, value).second) {
                return false;
            }
        }
        return true;
    }
    bool done() const {
        return cursor_ == bytes_.size();
    }

  private:
    const Bytes& bytes_;
    size_t cursor_ = 0;
};

void write_type(Writer& writer, const TypeRef& type) {
    writer.text(type.profile_id);
    writer.u16(type.major);
    writer.u16(type.minor);
    writer.text(type.type_name);
}

bool read_type(Reader& reader, TypeRef* type) {
    return reader.text(&type->profile_id) && reader.u16(&type->major) && reader.u16(&type->minor) &&
           reader.text(&type->type_name) && valid_type_ref(*type);
}

void write_entity(Writer& writer, const EntityDescriptor& entity) {
    writer.text(entity.entity_uid);
    writer.text(entity.kind);
    writer.text(entity.display_name);
    writer.text(entity.hardware_id);
    writer.map(entity.attributes);
    writer.list<std::string>(entity.capabilities,
                             [&](const auto& capability) { writer.text(capability); });
    writer.u8(static_cast<uint8_t>(entity.availability));
}

bool read_entity(Reader& reader, EntityDescriptor* entity) {
    uint8_t availability = 0;
    return reader.text(&entity->entity_uid) && valid_uid(entity->entity_uid) &&
           reader.text(&entity->kind) && reader.text(&entity->display_name) &&
           reader.text(&entity->hardware_id) && reader.string_map(&entity->attributes) &&
           reader.list<std::string>(&entity->capabilities,
                                    [&](auto* value) { return reader.text(value); }) &&
           reader.u8(&availability) &&
           availability <= static_cast<uint8_t>(Availability::kOffline) &&
           (entity->availability = static_cast<Availability>(availability), true);
}

template <typename T, typename Callback> bool decode_all(const Bytes& bytes, T* out, Callback cb) {
    if (out == nullptr) {
        return false;
    }
    Reader reader(bytes);
    T parsed;
    if (!cb(reader, &parsed) || !reader.done()) {
        return false;
    }
    *out = std::move(parsed);
    return true;
}

}  // namespace

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

}  // namespace yunlink::v2
