/**
 * @file src/core/semantic_messages_managed_entities.cpp
 * @brief Managed entity directory payload codecs.
 */

#include "semantic_codec_io.hpp"

namespace yunlink {
namespace {

constexpr uint16_t kMaxManagedEntities = 256;
constexpr uint16_t kMaxEntityCapabilities = 128;
constexpr uint16_t kMaxIdentityGroups = 128;

void write_identity(BufferWriter& writer, const EndpointIdentity& identity) {
    writer.write_u8(static_cast<uint8_t>(identity.agent_type));
    writer.write_u32(identity.agent_id);
    writer.write_u8(static_cast<uint8_t>(identity.role));
    if (identity.group_ids.size() > kMaxIdentityGroups) {
        writer.invalidate();
        return;
    }
    writer.write_u16(static_cast<uint16_t>(identity.group_ids.size()));
    for (uint32_t group_id : identity.group_ids) {
        writer.write_u32(group_id);
    }
}

bool read_identity(BufferReader& reader, EndpointIdentity* out) {
    uint8_t agent_type = 0;
    uint8_t role = 0;
    uint16_t group_count = 0;
    if (out == nullptr || !reader.read_u8(&agent_type) || !reader.read_u32(&out->agent_id) ||
        !reader.read_u8(&role) || !reader.read_u16(&group_count) ||
        group_count > kMaxIdentityGroups) {
        return false;
    }
    out->agent_type = static_cast<AgentType>(agent_type);
    out->role = static_cast<EndpointRole>(role);
    out->group_ids.clear();
    out->group_ids.reserve(group_count);
    for (uint16_t index = 0; index < group_count; ++index) {
        uint32_t group_id = 0;
        if (!reader.read_u32(&group_id)) {
            return false;
        }
        out->group_ids.push_back(group_id);
    }
    return true;
}

void write_entity(BufferWriter& writer, const ManagedEntityDescriptor& entity) {
    writer.write_string(entity.entity_uid);
    write_identity(writer, entity.identity);
    writer.write_string(entity.display_name);
    writer.write_string(entity.hardware_id);
    if (entity.capabilities.size() > kMaxEntityCapabilities) {
        writer.invalidate();
        return;
    }
    writer.write_u16(static_cast<uint16_t>(entity.capabilities.size()));
    for (const std::string& capability : entity.capabilities) {
        writer.write_string(capability);
    }
    writer.write_u8(static_cast<uint8_t>(entity.availability));
}

bool read_entity(BufferReader& reader, ManagedEntityDescriptor* out) {
    uint16_t capability_count = 0;
    uint8_t availability = 0;
    if (out == nullptr || !reader.read_string(&out->entity_uid) ||
        !read_identity(reader, &out->identity) || !reader.read_string(&out->display_name) ||
        !reader.read_string(&out->hardware_id) || !reader.read_u16(&capability_count) ||
        capability_count > kMaxEntityCapabilities) {
        return false;
    }
    out->capabilities.clear();
    out->capabilities.reserve(capability_count);
    for (uint16_t index = 0; index < capability_count; ++index) {
        std::string capability;
        if (!reader.read_string(&capability)) {
            return false;
        }
        out->capabilities.push_back(std::move(capability));
    }
    if (!reader.read_u8(&availability)) {
        return false;
    }
    out->availability = static_cast<ManagedEntityAvailability>(availability);
    return true;
}

}  // namespace

ByteBuffer encode_payload(const ManagedEntityListRequest& payload) {
    return build_payload([&](BufferWriter& writer) { writer.write_u8(payload.reserved); });
}

bool decode_payload(const ByteBuffer& bytes, ManagedEntityListRequest* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ManagedEntityListRequest* out) {
        return reader.read_u8(&out->reserved);
    });
}

ByteBuffer encode_payload(const ManagedEntityListResponse& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_bool(payload.success);
        writer.write_string(payload.message);
        writer.write_string(payload.endpoint_uid);
        writer.write_string(payload.revision);
        write_identity(writer, payload.primary_identity);
        if (payload.entities.size() > kMaxManagedEntities) {
            writer.invalidate();
            return;
        }
        writer.write_u16(static_cast<uint16_t>(payload.entities.size()));
        for (const ManagedEntityDescriptor& entity : payload.entities) {
            write_entity(writer, entity);
        }
    });
}

bool decode_payload(const ByteBuffer& bytes, ManagedEntityListResponse* payload) {
    return parse_payload(bytes, payload, [](BufferReader& reader, ManagedEntityListResponse* out) {
        uint16_t entity_count = 0;
        if (!reader.read_bool(&out->success) || !reader.read_string(&out->message) ||
            !reader.read_string(&out->endpoint_uid) || !reader.read_string(&out->revision) ||
            !read_identity(reader, &out->primary_identity) || !reader.read_u16(&entity_count) ||
            entity_count > kMaxManagedEntities) {
            return false;
        }
        out->entities.clear();
        out->entities.reserve(entity_count);
        for (uint16_t index = 0; index < entity_count; ++index) {
            ManagedEntityDescriptor entity;
            if (!read_entity(reader, &entity)) {
                return false;
            }
            out->entities.push_back(std::move(entity));
        }
        return true;
    });
}

ByteBuffer encode_payload(const ManagedEntityDirectoryChanged& payload) {
    return build_payload([&](BufferWriter& writer) {
        writer.write_string(payload.endpoint_uid);
        writer.write_string(payload.revision);
    });
}

bool decode_payload(const ByteBuffer& bytes, ManagedEntityDirectoryChanged* payload) {
    return parse_payload(
        bytes, payload, [](BufferReader& reader, ManagedEntityDirectoryChanged* out) {
            return reader.read_string(&out->endpoint_uid) && reader.read_string(&out->revision);
        });
}

}  // namespace yunlink
