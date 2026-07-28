/** @file @brief Shared configuration resource codec helpers. */

#ifndef YUNLINK_CORE_CONFIGURATION_SERVICE_CODEC_IO_HPP
#define YUNLINK_CORE_CONFIGURATION_SERVICE_CODEC_IO_HPP

#include "../binary_codec_io.hpp"

#include <cmath>
#include <utility>

namespace yunlink::configuration_codec {

constexpr uint16_t kMaxConfigItems = 256;

template <typename T, typename WriteFn>
void write_vector(BufferWriter& writer, const std::vector<T>& values, WriteFn&& write) {
    if (values.size() > kMaxConfigItems) {
        writer.invalidate();
        return;
    }
    writer.write_u16(static_cast<uint16_t>(values.size()));
    for (const auto& value : values) {
        write(writer, value);
    }
}

template <typename T, typename ReadFn>
bool read_vector(BufferReader& reader, std::vector<T>* out, ReadFn&& read) {
    uint16_t count = 0;
    if (out == nullptr || !reader.read_u16(&count) || count > kMaxConfigItems) {
        return false;
    }
    out->clear();
    out->reserve(count);
    for (uint16_t index = 0; index < count; ++index) {
        T value{};
        if (!read(reader, &value)) {
            return false;
        }
        out->push_back(std::move(value));
    }
    return true;
}

inline void write_strings(BufferWriter& writer, const std::vector<std::string>& values) {
    write_vector(writer, values, [](BufferWriter& target, const std::string& value) {
        target.write_string(value);
    });
}

inline bool read_strings(BufferReader& reader, std::vector<std::string>* out) {
    return read_vector(reader, out, [](BufferReader& source, std::string* value) {
        return source.read_string(value);
    });
}

inline bool valid_value_type(uint8_t value) {
    return value >= static_cast<uint8_t>(ConfigValueType::kBool) &&
           value <= static_cast<uint8_t>(ConfigValueType::kDoubleList);
}

inline bool valid_status(uint8_t value) {
    return value <= static_cast<uint8_t>(ConfigServiceStatus::kInternalError);
}

inline bool valid_requirement(uint8_t value) {
    return value <= static_cast<uint8_t>(ConfigApplyRequirement::kManual);
}

inline bool valid_outcome(uint8_t value) {
    return value >= static_cast<uint8_t>(ConfigApplyOutcome::kApplied) &&
           value <= static_cast<uint8_t>(ConfigApplyOutcome::kFailed);
}

inline bool valid_update_policy(uint8_t value) {
    return value <= static_cast<uint8_t>(ConfigFieldUpdatePolicy::kManual);
}

inline bool valid_variant_source(uint8_t value) {
    return value >= static_cast<uint8_t>(ConfigVariantSource::kDefault) &&
           value <= static_cast<uint8_t>(ConfigVariantSource::kActive);
}

inline void write_value(BufferWriter& writer, const ConfigValue& value) {
    writer.write_u8(static_cast<uint8_t>(value.type));
    switch (value.type) {
    case ConfigValueType::kBool:
        writer.write_bool(value.bool_value);
        return;
    case ConfigValueType::kInt64:
        writer.write_u64(static_cast<uint64_t>(value.int64_value));
        return;
    case ConfigValueType::kDouble:
        if (!std::isfinite(value.double_value)) {
            writer.invalidate();
            return;
        }
        writer.write_double(value.double_value);
        return;
    case ConfigValueType::kString:
        writer.write_string(value.string_value);
        return;
    case ConfigValueType::kStringList:
        write_strings(writer, value.string_list_value);
        return;
    case ConfigValueType::kDoubleList:
        write_vector(writer, value.double_list_value, [](BufferWriter& target, double item) {
            if (!std::isfinite(item)) {
                target.invalidate();
                return;
            }
            target.write_double(item);
        });
        return;
    }
}

inline bool read_value(BufferReader& reader, ConfigValue* out) {
    uint8_t type = 0;
    if (out == nullptr || !reader.read_u8(&type) || !valid_value_type(type)) {
        return false;
    }
    *out = ConfigValue{};
    out->type = static_cast<ConfigValueType>(type);
    switch (out->type) {
    case ConfigValueType::kBool:
        return reader.read_bool(&out->bool_value);
    case ConfigValueType::kInt64: {
        uint64_t raw = 0;
        if (!reader.read_u64(&raw)) {
            return false;
        }
        out->int64_value = static_cast<int64_t>(raw);
        return true;
    }
    case ConfigValueType::kDouble:
        return reader.read_double(&out->double_value) && std::isfinite(out->double_value);
    case ConfigValueType::kString:
        return reader.read_string(&out->string_value);
    case ConfigValueType::kStringList:
        return read_strings(reader, &out->string_list_value);
    case ConfigValueType::kDoubleList:
        return read_vector(reader, &out->double_list_value, [](BufferReader& source, double* item) {
            return source.read_double(item) && std::isfinite(*item);
        });
    }
    return false;
}

inline void write_descriptor(BufferWriter& writer, const ConfigResourceDescriptor& value) {
    writer.write_string(value.id);
    writer.write_string(value.title);
    writer.write_string(value.description);
    writer.write_bool(value.readable);
    writer.write_bool(value.writable);
    writer.write_bool(value.apply_supported);
    writer.write_bool(value.variants_supported);
}

inline bool read_descriptor(BufferReader& reader, ConfigResourceDescriptor* out) {
    return out != nullptr && reader.read_string(&out->id) && reader.read_string(&out->title) &&
           reader.read_string(&out->description) && reader.read_bool(&out->readable) &&
           reader.read_bool(&out->writable) && reader.read_bool(&out->apply_supported) &&
           reader.read_bool(&out->variants_supported);
}

inline void write_choice(BufferWriter& writer, const ConfigChoice& value) {
    write_value(writer, value.value);
    writer.write_string(value.label);
}

inline bool read_choice(BufferReader& reader, ConfigChoice* out) {
    return out != nullptr && read_value(reader, &out->value) && reader.read_string(&out->label);
}

inline void write_schema(BufferWriter& writer, const ConfigFieldSchema& value) {
    writer.write_string(value.path);
    writer.write_string(value.title);
    writer.write_string(value.description);
    writer.write_u8(static_cast<uint8_t>(value.type));
    writer.write_bool(value.required);
    writer.write_bool(value.read_only);
    writer.write_bool(value.sensitive);
    writer.write_bool(value.has_minimum);
    writer.write_double(value.minimum);
    writer.write_bool(value.has_maximum);
    writer.write_double(value.maximum);
    writer.write_string(value.validation_pattern);
    write_vector(writer, value.choices, write_choice);
    writer.write_string(value.group_path);
    writer.write_u8(static_cast<uint8_t>(value.update_policy));
    writer.write_string(value.unit);
}

inline bool read_schema(BufferReader& reader, ConfigFieldSchema* out) {
    uint8_t type = 0;
    if (out == nullptr || !reader.read_string(&out->path) || !reader.read_string(&out->title) ||
        !reader.read_string(&out->description) || !reader.read_u8(&type) ||
        !valid_value_type(type)) {
        return false;
    }
    out->type = static_cast<ConfigValueType>(type);
    uint8_t update_policy = 0;
    if (!(reader.read_bool(&out->required) && reader.read_bool(&out->read_only) &&
           reader.read_bool(&out->sensitive) && reader.read_bool(&out->has_minimum) &&
           reader.read_double(&out->minimum) && reader.read_bool(&out->has_maximum) &&
           reader.read_double(&out->maximum) && reader.read_string(&out->validation_pattern) &&
           read_vector(reader, &out->choices, read_choice) && reader.read_string(&out->group_path) &&
           reader.read_u8(&update_policy) && reader.read_string(&out->unit))) {
        return false;
    }
    if (!std::isfinite(out->minimum) || !std::isfinite(out->maximum) ||
        !valid_update_policy(update_policy)) {
        return false;
    }
    out->update_policy = static_cast<ConfigFieldUpdatePolicy>(update_policy);
    return true;
}

inline void write_field_value(BufferWriter& writer, const ConfigFieldValue& value) {
    writer.write_string(value.path);
    write_value(writer, value.value);
}

inline bool read_field_value(BufferReader& reader, ConfigFieldValue* out) {
    return out != nullptr && reader.read_string(&out->path) && read_value(reader, &out->value);
}

inline void write_snapshot(BufferWriter& writer, const ConfigSnapshot& value) {
    writer.write_string(value.resource_id);
    writer.write_string(value.revision);
    writer.write_string(value.applied_revision);
    writer.write_string(value.variant_id);
    writer.write_string(value.active_variant_id);
    write_vector(writer, value.values, write_field_value);
}

inline bool read_snapshot(BufferReader& reader, ConfigSnapshot* out) {
    return out != nullptr && reader.read_string(&out->resource_id) &&
           reader.read_string(&out->revision) && reader.read_string(&out->applied_revision) &&
           reader.read_string(&out->variant_id) && reader.read_string(&out->active_variant_id) &&
           read_vector(reader, &out->values, read_field_value);
}

inline void write_variant(BufferWriter& writer, const ConfigVariantDescriptor& value) {
    writer.write_string(value.id);
    writer.write_string(value.title);
    writer.write_string(value.revision);
    writer.write_u64(value.modified_at_ns);
    writer.write_bool(value.active);
    writer.write_bool(value.mutable_variant);
}

inline bool read_variant(BufferReader& reader, ConfigVariantDescriptor* out) {
    return out != nullptr && reader.read_string(&out->id) && reader.read_string(&out->title) &&
           reader.read_string(&out->revision) && reader.read_u64(&out->modified_at_ns) &&
           reader.read_bool(&out->active) && reader.read_bool(&out->mutable_variant);
}

inline void write_field_error(BufferWriter& writer, const ConfigFieldError& value) {
    writer.write_string(value.path);
    writer.write_string(value.code);
    writer.write_string(value.message);
}

inline bool read_field_error(BufferReader& reader, ConfigFieldError* out) {
    return out != nullptr && reader.read_string(&out->path) && reader.read_string(&out->code) &&
           reader.read_string(&out->message);
}

inline void write_effects(BufferWriter& writer, const ConfigEffects& value) {
    writer.write_u8(static_cast<uint8_t>(value.requirement));
    write_strings(writer, value.affected_components);
    writer.write_bool(value.reconnect_expected);
}

inline bool read_effects(BufferReader& reader, ConfigEffects* out) {
    uint8_t requirement = 0;
    if (out == nullptr || !reader.read_u8(&requirement) || !valid_requirement(requirement)) {
        return false;
    }
    out->requirement = static_cast<ConfigApplyRequirement>(requirement);
    return read_strings(reader, &out->affected_components) &&
           reader.read_bool(&out->reconnect_expected);
}

inline void
write_status(BufferWriter& writer, ConfigServiceStatus status, const std::string& message) {
    writer.write_u8(static_cast<uint8_t>(status));
    writer.write_string(message);
}

inline bool read_status(BufferReader& reader, ConfigServiceStatus* status, std::string* message) {
    uint8_t raw = 0;
    if (status == nullptr || message == nullptr || !reader.read_u8(&raw) || !valid_status(raw) ||
        !reader.read_string(message)) {
        return false;
    }
    *status = static_cast<ConfigServiceStatus>(raw);
    return true;
}

}  // namespace yunlink::configuration_codec

#endif  // YUNLINK_CORE_CONFIGURATION_SERVICE_CODEC_IO_HPP
