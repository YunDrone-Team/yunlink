/**
 * @file src/core/configuration_service/codec.cpp
 * @brief Typed configuration resource payload codec.
 */

#include "codec_io.hpp"

namespace yunlink {
using namespace configuration_codec;

ByteBuffer encode_payload(const ConfigResourceListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u8(payload.reserved); });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceListRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourceListRequest* out) {
        return reader.read_u8(&out->reserved);
    });
}

ByteBuffer encode_payload(const ConfigResourceListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_vector(writer, payload.resources, write_descriptor);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceListResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourceListResponse* out) {
        return read_status(reader, &out->status, &out->message) &&
               read_vector(reader, &out->resources, read_descriptor);
    });
}

ByteBuffer encode_payload(const ConfigResourceDescribeRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_string(payload.resource_id); });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceDescribeRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceDescribeRequest* out) {
            return reader.read_string(&out->resource_id);
        });
}

ByteBuffer encode_payload(const ConfigResourceDescribeResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_descriptor(writer, payload.resource);
        write_vector(writer, payload.fields, write_schema);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceDescribeResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceDescribeResponse* out) {
            return read_status(reader, &out->status, &out->message) &&
                   read_descriptor(reader, &out->resource) &&
                   read_vector(reader, &out->fields, read_schema);
        });
}

ByteBuffer encode_payload(const ConfigResourceGetRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceGetRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourceGetRequest* out) {
        return reader.read_string(&out->resource_id) && reader.read_string(&out->variant_id);
    });
}

ByteBuffer encode_payload(const ConfigResourceGetResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_snapshot(writer, payload.snapshot);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceGetResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourceGetResponse* out) {
        return read_status(reader, &out->status, &out->message) &&
               read_snapshot(reader, &out->snapshot);
    });
}

ByteBuffer encode_payload(const ConfigResourcePatchRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
        writer.write_string(payload.expected_revision);
        write_vector(writer, payload.updates, write_field_value);
        writer.write_bool(payload.validate_only);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourcePatchRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourcePatchRequest* out) {
        return reader.read_string(&out->resource_id) && reader.read_string(&out->variant_id) &&
               reader.read_string(&out->expected_revision) &&
               read_vector(reader, &out->updates, read_field_value) &&
               reader.read_bool(&out->validate_only);
    });
}

ByteBuffer encode_payload(const ConfigResourcePatchResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_snapshot(writer, payload.snapshot);
        writer.write_bool(payload.has_candidate_snapshot);
        if (payload.has_candidate_snapshot) {
            write_snapshot(writer, payload.candidate_snapshot);
        }
        write_vector(writer, payload.errors, write_field_error);
        write_effects(writer, payload.effects);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourcePatchResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourcePatchResponse* out) {
            return read_status(reader, &out->status, &out->message) &&
                   read_snapshot(reader, &out->snapshot) &&
                   reader.read_bool(&out->has_candidate_snapshot) &&
                   (!out->has_candidate_snapshot ||
                    read_snapshot(reader, &out->candidate_snapshot)) &&
                   read_vector(reader, &out->errors, read_field_error) &&
                   read_effects(reader, &out->effects);
        });
}

ByteBuffer encode_payload(const ConfigResourceApplyRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.expected_revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceApplyRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ConfigResourceApplyRequest* out) {
        return reader.read_string(&out->resource_id) && reader.read_string(&out->expected_revision);
    });
}

ByteBuffer encode_payload(const ConfigResourceApplyResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        writer.write_string(payload.applied_revision);
        writer.write_u8(static_cast<uint8_t>(payload.outcome));
        write_effects(writer, payload.effects);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceApplyResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceApplyResponse* out) {
            uint8_t outcome = 0;
            if (!read_status(reader, &out->status, &out->message) ||
                !reader.read_string(&out->applied_revision) || !reader.read_u8(&outcome) ||
                !valid_outcome(outcome)) {
                return false;
            }
            out->outcome = static_cast<ConfigApplyOutcome>(outcome);
            return read_effects(reader, &out->effects);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_string(payload.resource_id); });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantListRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantListRequest* out) {
            return reader.read_string(&out->resource_id);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        writer.write_string(payload.active_variant_id);
        write_vector(writer, payload.variants, write_variant);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantListResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantListResponse* out) {
            return read_status(reader, &out->status, &out->message) &&
                   reader.read_string(&out->active_variant_id) &&
                   read_vector(reader, &out->variants, read_variant);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantCreateRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
        writer.write_u8(static_cast<uint8_t>(payload.source));
        writer.write_string(payload.expected_active_revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantCreateRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantCreateRequest* out) {
            uint8_t source = 0;
            if (!reader.read_string(&out->resource_id) || !reader.read_string(&out->variant_id) ||
                !reader.read_u8(&source) || !valid_variant_source(source) ||
                !reader.read_string(&out->expected_active_revision)) {
                return false;
            }
            out->source = static_cast<ConfigVariantSource>(source);
            return true;
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantCreateResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_variant(writer, payload.variant);
        write_effects(writer, payload.effects);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantCreateResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantCreateResponse* out) {
            return read_status(reader, &out->status, &out->message) &&
                   read_variant(reader, &out->variant) && read_effects(reader, &out->effects);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantSaveCurrentRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
        writer.write_string(payload.expected_variant_revision);
        writer.write_string(payload.expected_active_revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantSaveCurrentRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantSaveCurrentRequest* out) {
            return reader.read_string(&out->resource_id) && reader.read_string(&out->variant_id) &&
                   reader.read_string(&out->expected_variant_revision) &&
                   reader.read_string(&out->expected_active_revision);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantSaveCurrentResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        write_variant(writer, payload.variant);
        write_effects(writer, payload.effects);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantSaveCurrentResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantSaveCurrentResponse* out) {
            return read_status(reader, &out->status, &out->message) &&
                   read_variant(reader, &out->variant) && read_effects(reader, &out->effects);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantActivateRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
        writer.write_string(payload.expected_active_revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantActivateRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantActivateRequest* out) {
            return reader.read_string(&out->resource_id) && reader.read_string(&out->variant_id) &&
                   reader.read_string(&out->expected_active_revision);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantActivateResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        write_status(writer, payload.status, payload.message);
        writer.write_string(payload.applied_revision);
        writer.write_u8(static_cast<uint8_t>(payload.outcome));
        write_effects(writer, payload.effects);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantActivateResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantActivateResponse* out) {
            uint8_t outcome = 0;
            if (!read_status(reader, &out->status, &out->message) ||
                !reader.read_string(&out->applied_revision) || !reader.read_u8(&outcome) ||
                !valid_outcome(outcome)) {
                return false;
            }
            out->outcome = static_cast<ConfigApplyOutcome>(outcome);
            return read_effects(reader, &out->effects);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantDeleteRequest& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.resource_id);
        writer.write_string(payload.variant_id);
        writer.write_string(payload.expected_revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantDeleteRequest* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantDeleteRequest* out) {
            return reader.read_string(&out->resource_id) && reader.read_string(&out->variant_id) &&
                   reader.read_string(&out->expected_revision);
        });
}

ByteBuffer encode_payload(const ConfigResourceVariantDeleteResponse& payload) {
    return build_payload(
        [&](BufferWriter& writer) { write_status(writer, payload.status, payload.message); });
}

bool decode_payload(const ByteBuffer& bytes, ConfigResourceVariantDeleteResponse* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ConfigResourceVariantDeleteResponse* out) {
            return read_status(reader, &out->status, &out->message);
        });
}

}  // namespace yunlink
