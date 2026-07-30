#pragma once
#include "yunlink/core/core_messages_v2.hpp"

#include <cstddef>
#include <cstring>
#include <limits>
#include <utility>

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
    bool bytes(const Bytes& value) {
        if (value.size() > UINT32_MAX) {
            valid_ = false;
            return false;
        }
        u32(static_cast<uint32_t>(value.size()));
        data_.insert(data_.end(), value.begin(), value.end());
        return true;
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
    bool bytes(Bytes* value) {
        uint32_t size = 0;
        if (value == nullptr || !u32(&size) || cursor_ + size > bytes_.size()) {
            return false;
        }
        value->assign(bytes_.begin() + static_cast<std::ptrdiff_t>(cursor_),
                      bytes_.begin() + static_cast<std::ptrdiff_t>(cursor_ + size));
        cursor_ += size;
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

void write_log_summary(Writer& writer, const LogSummary& value) {
    writer.text(value.log_uid);
    writer.text(value.owner_uid);
    writer.text(value.title);
    writer.text(value.state);
    writer.u64(value.started_at_ns);
    writer.u64(value.finished_at_ns);
    writer.u8(value.has_exit_code ? 1 : 0);
    writer.u32(static_cast<uint32_t>(value.exit_code));
    writer.map(value.labels);
    writer.text(value.message);
}

bool read_log_summary(Reader& reader, LogSummary* value) {
    uint8_t has_exit_code = 0;
    uint32_t exit_code = 0;
    return value != nullptr && reader.text(&value->log_uid) && valid_uid(value->log_uid) &&
           reader.text(&value->owner_uid) && valid_uid(value->owner_uid) &&
           reader.text(&value->title) && reader.text(&value->state) &&
           reader.u64(&value->started_at_ns) && reader.u64(&value->finished_at_ns) &&
           reader.u8(&has_exit_code) && has_exit_code <= 1 &&
           (value->has_exit_code = has_exit_code != 0, true) && reader.u32(&exit_code) &&
           (value->exit_code = static_cast<int32_t>(exit_code), true) &&
           reader.string_map(&value->labels) && reader.text(&value->message);
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
}  // namespace yunlink::v2
