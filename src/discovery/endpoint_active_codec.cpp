/** @file @brief Authenticated active-discovery query and reply codec. */

#include "yunlink/discovery/endpoint_discovery.hpp"

#include <algorithm>
#include <cstring>
#include <utility>

namespace yunlink {
namespace {

void set_error(std::string* error, const std::string& value) {
    if (error != nullptr) {
        *error = value;
    }
}

uint64_t fnv1a64(const std::string& secret, const ByteBuffer& bytes) {
    uint64_t value = 14695981039346656037ULL;
    const auto mix = [&value](uint8_t byte) {
        value ^= byte;
        value *= 1099511628211ULL;
    };
    for (unsigned char byte : secret) {
        mix(byte);
    }
    for (uint8_t byte : bytes) {
        mix(byte);
    }
    return value;
}

void append_u16(ByteBuffer* out, uint16_t value) {
    out->push_back(static_cast<uint8_t>((value >> 8U) & 0xffU));
    out->push_back(static_cast<uint8_t>(value & 0xffU));
}

void append_u32(ByteBuffer* out, uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) {
        out->push_back(static_cast<uint8_t>((value >> shift) & 0xffU));
    }
}

void append_u64(ByteBuffer* out, uint64_t value) {
    for (int shift = 56; shift >= 0; shift -= 8) {
        out->push_back(static_cast<uint8_t>((value >> shift) & 0xffU));
    }
}

bool take_u16(const ByteBuffer& bytes, std::size_t* offset, uint16_t* value) {
    if (*offset + 2U > bytes.size()) {
        return false;
    }
    *value = static_cast<uint16_t>((static_cast<uint16_t>(bytes[*offset]) << 8U) |
                                   static_cast<uint16_t>(bytes[*offset + 1U]));
    *offset += 2U;
    return true;
}

bool take_u32(const ByteBuffer& bytes, std::size_t* offset, uint32_t* value) {
    if (*offset + 4U > bytes.size()) {
        return false;
    }
    *value = 0;
    for (int index = 0; index < 4; ++index) {
        *value = (*value << 8U) | bytes[*offset + static_cast<std::size_t>(index)];
    }
    *offset += 4U;
    return true;
}

bool take_u64(const ByteBuffer& bytes, std::size_t* offset, uint64_t* value) {
    if (*offset + 8U > bytes.size()) {
        return false;
    }
    *value = 0;
    for (int index = 0; index < 8; ++index) {
        *value = (*value << 8U) | bytes[*offset + static_cast<std::size_t>(index)];
    }
    *offset += 8U;
    return true;
}

bool append_text(ByteBuffer* out, const std::string& value, std::size_t max_length) {
    if (value.size() > max_length || value.size() > 255U) {
        return false;
    }
    out->push_back(static_cast<uint8_t>(value.size()));
    out->insert(out->end(), value.begin(), value.end());
    return true;
}

bool take_text(const ByteBuffer& bytes, std::size_t* offset, std::string* value) {
    if (*offset >= bytes.size()) {
        return false;
    }
    const std::size_t length = bytes[*offset];
    ++*offset;
    if (*offset + length > bytes.size()) {
        return false;
    }
    value->assign(reinterpret_cast<const char*>(bytes.data() + *offset), length);
    *offset += length;
    return true;
}

constexpr uint8_t kManagedEntitySummaryExtensionMarker = 0xe1U;
constexpr std::size_t kMaxActiveDiscoveryReplySize = 1024U;

bool append_managed_entity_summary(ByteBuffer* out, const EndpointManagedEntitySummary& summary) {
    if (!append_text(out, summary.entity_uid, 96U) || !append_text(out, summary.agent_type, 15U)) {
        return false;
    }
    append_u32(out, summary.agent_id);
    return append_text(out, summary.display_name, 96U) && append_text(out, summary.node_name, 96U);
}

bool take_managed_entity_summary(const ByteBuffer& bytes,
                                 std::size_t* offset,
                                 EndpointManagedEntitySummary* summary) {
    return take_text(bytes, offset, &summary->entity_uid) &&
           take_text(bytes, offset, &summary->agent_type) &&
           take_u32(bytes, offset, &summary->agent_id) &&
           take_text(bytes, offset, &summary->display_name) &&
           take_text(bytes, offset, &summary->node_name) && !summary->entity_uid.empty() &&
           !summary->agent_type.empty() && summary->agent_id != 0U;
}

uint16_t capability_mask(const std::vector<std::string>& capabilities) {
    uint16_t mask = 0;
    for (const auto& capability : capabilities) {
        if (capability == "state") {
            mask |= 1U;
        } else if (capability == "commands") {
            mask |= 2U;
        } else if (capability == "system_service") {
            mask |= 4U;
        } else if (capability == "config-resource-v1") {
            mask |= 8U;
        } else if (capability == "topic-stream-v1") {
            mask |= 16U;
        } else if (capability == "managed-entities") {
            mask |= 32U;
        }
    }
    return mask;
}

std::vector<std::string> capabilities_from_mask(uint16_t mask) {
    std::vector<std::string> result;
    if ((mask & 1U) != 0U)
        result.push_back("state");
    if ((mask & 2U) != 0U)
        result.push_back("commands");
    if ((mask & 4U) != 0U)
        result.push_back("system_service");
    if ((mask & 8U) != 0U)
        result.push_back("config-resource-v1");
    if ((mask & 16U) != 0U)
        result.push_back("topic-stream-v1");
    if ((mask & 32U) != 0U)
        result.push_back("managed-entities");
    return result;
}

void append_tag(ByteBuffer* out, const std::string& secret) {
    const uint64_t tag = fnv1a64(secret, *out);
    append_u64(out, tag);
}

bool verify_tag(const ByteBuffer& bytes, const std::string& secret) {
    if (bytes.size() < 8U) {
        return false;
    }
    ByteBuffer signed_bytes(bytes.begin(), bytes.end() - 8);
    std::size_t offset = bytes.size() - 8U;
    uint64_t supplied = 0;
    return take_u64(bytes, &offset, &supplied) && supplied == fnv1a64(secret, signed_bytes);
}

}  // namespace

ByteBuffer encode_endpoint_discovery_query(const EndpointDiscoveryQuery& query,
                                           const std::string& shared_secret) {
    ByteBuffer out(kEndpointDiscoveryQueryMagic,
                   kEndpointDiscoveryQueryMagic + std::strlen(kEndpointDiscoveryQueryMagic));
    append_u64(&out, query.nonce);
    append_u16(&out, query.response_window_ms);
    append_tag(&out, shared_secret);
    return out;
}

bool decode_endpoint_discovery_query(const ByteBuffer& bytes,
                                     const std::string& shared_secret,
                                     EndpointDiscoveryQuery* out,
                                     std::string* error) {
    if (out == nullptr || bytes.size() != 22U ||
        !std::equal(bytes.begin(), bytes.begin() + 4, kEndpointDiscoveryQueryMagic) ||
        !verify_tag(bytes, shared_secret)) {
        set_error(error, "invalid discovery query");
        return false;
    }
    std::size_t offset = 4;
    if (!take_u64(bytes, &offset, &out->nonce) ||
        !take_u16(bytes, &offset, &out->response_window_ms) || out->nonce == 0 ||
        out->response_window_ms < 50U || out->response_window_ms > 2000U) {
        set_error(error, "invalid discovery query fields");
        return false;
    }
    return true;
}

ByteBuffer encode_endpoint_discovery_reply(uint64_t nonce,
                                           const EndpointAdvertisement& advertisement,
                                           const std::string& shared_secret) {
    if (nonce == 0 || !validate_endpoint_id(advertisement.endpoint_id)) {
        return {};
    }
    ByteBuffer out(kEndpointDiscoveryReplyMagic,
                   kEndpointDiscoveryReplyMagic + std::strlen(kEndpointDiscoveryReplyMagic));
    append_u64(&out, nonce);
    if (!append_text(&out, advertisement.endpoint_id, 16U) ||
        !append_text(&out, advertisement.display_name_prefix, 32U) ||
        !append_text(&out, advertisement.agent_type, 15U) ||
        !append_text(&out, advertisement.role, 15U) ||
        !append_text(&out, advertisement.node_name, 63U)) {
        return {};
    }
    append_u32(&out, advertisement.agent_id);
    append_u16(&out, advertisement.tcp_listen_port);
    append_u16(&out, advertisement.udp_bind_port);
    append_u16(&out, capability_mask(advertisement.capabilities));
    append_u64(&out, advertisement.sequence);
    append_u32(&out, advertisement.discovery_period_ms);
    if (advertisement.managed_entity_count_known) {
        // Keep active discovery replies within their datagram budget even when a
        // Bridge has long display names. The total stays authoritative while the
        // optional preview is shortened to what safely fits in this reply.
        ByteBuffer extension;
        extension.push_back(kManagedEntitySummaryExtensionMarker);
        append_u16(&extension, advertisement.managed_entity_count);
        extension.push_back(0U);
        std::size_t summary_count = 0U;
        const std::size_t summary_limit =
            std::min(advertisement.managed_entities.size(), kMaxDiscoveryManagedEntitySummaries);
        for (std::size_t index = 0; index < summary_limit; ++index) {
            ByteBuffer candidate = extension;
            if (!append_managed_entity_summary(&candidate, advertisement.managed_entities[index])) {
                return {};
            }
            if (out.size() + candidate.size() + 8U > kMaxActiveDiscoveryReplySize) {
                break;
            }
            extension = std::move(candidate);
            ++summary_count;
        }
        extension[3] = static_cast<uint8_t>(summary_count);
        out.insert(out.end(), extension.begin(), extension.end());
    }
    if (out.size() + 8U > kMaxActiveDiscoveryReplySize) {
        return {};
    }
    append_tag(&out, shared_secret);
    return out;
}

bool decode_endpoint_discovery_reply(const ByteBuffer& bytes,
                                     const std::string& shared_secret,
                                     uint64_t* nonce,
                                     EndpointAdvertisement* out,
                                     std::string* error) {
    if (nonce == nullptr || out == nullptr || bytes.size() < 47U ||
        bytes.size() > kMaxActiveDiscoveryReplySize ||
        !std::equal(bytes.begin(), bytes.begin() + 4, kEndpointDiscoveryReplyMagic) ||
        !verify_tag(bytes, shared_secret)) {
        set_error(error, "invalid discovery reply");
        return false;
    }
    std::size_t offset = 4;
    EndpointAdvertisement decoded{};
    uint16_t capability_bits = 0;
    if (!take_u64(bytes, &offset, nonce) || !take_text(bytes, &offset, &decoded.endpoint_id) ||
        !take_text(bytes, &offset, &decoded.display_name_prefix) ||
        !take_text(bytes, &offset, &decoded.agent_type) ||
        !take_text(bytes, &offset, &decoded.role) ||
        !take_text(bytes, &offset, &decoded.node_name) ||
        !take_u32(bytes, &offset, &decoded.agent_id) ||
        !take_u16(bytes, &offset, &decoded.tcp_listen_port) ||
        !take_u16(bytes, &offset, &decoded.udp_bind_port) ||
        !take_u16(bytes, &offset, &capability_bits) ||
        !take_u64(bytes, &offset, &decoded.sequence) ||
        !take_u32(bytes, &offset, &decoded.discovery_period_ms) || offset > bytes.size() - 8U ||
        *nonce == 0 || !validate_endpoint_id(decoded.endpoint_id) || decoded.agent_type.empty() ||
        decoded.agent_type.size() > 15U || decoded.role.empty() || decoded.role.size() > 15U) {
        set_error(error, "invalid discovery reply fields");
        return false;
    }
    decoded.protocol_version = "0.1.0";
    decoded.capabilities = capabilities_from_mask(capability_bits);
    const std::size_t signed_end = bytes.size() - 8U;
    if (offset < signed_end) {
        if (bytes[offset++] != kManagedEntitySummaryExtensionMarker) {
            set_error(error, "unknown discovery reply extension");
            return false;
        }
        uint16_t entity_count = 0;
        if (!take_u16(bytes, &offset, &entity_count) || offset >= signed_end) {
            set_error(error, "invalid managed entity extension");
            return false;
        }
        const std::size_t summary_count = bytes[offset++];
        if (summary_count > kMaxDiscoveryManagedEntitySummaries || summary_count > entity_count) {
            set_error(error, "invalid managed entity summary count");
            return false;
        }
        decoded.managed_entity_count_known = true;
        decoded.managed_entity_count = entity_count;
        decoded.managed_entities.reserve(summary_count);
        for (std::size_t index = 0; index < summary_count; ++index) {
            EndpointManagedEntitySummary summary{};
            if (!take_managed_entity_summary(bytes, &offset, &summary)) {
                set_error(error, "invalid managed entity summary");
                return false;
            }
            decoded.managed_entities.push_back(std::move(summary));
        }
    }
    if (offset != signed_end) {
        set_error(error, "trailing discovery reply bytes");
        return false;
    }
    decoded.display_name =
        make_endpoint_display_name(decoded.display_name_prefix, decoded.endpoint_id);
    *out = std::move(decoded);
    return true;
}

}  // namespace yunlink
