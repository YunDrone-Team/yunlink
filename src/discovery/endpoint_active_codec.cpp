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
    ByteBuffer out(kEndpointDiscoveryReplyMagic,
                   kEndpointDiscoveryReplyMagic + std::strlen(kEndpointDiscoveryReplyMagic));
    append_u64(&out, nonce);
    append_text(&out, advertisement.endpoint_id, 16U);
    append_text(&out, advertisement.display_name_prefix, 32U);
    append_text(&out, advertisement.node_name, 63U);
    append_u32(&out, advertisement.agent_id);
    append_u16(&out, advertisement.tcp_listen_port);
    append_u16(&out, advertisement.udp_bind_port);
    append_u16(&out, capability_mask(advertisement.capabilities));
    append_u64(&out, advertisement.sequence);
    append_u32(&out, advertisement.discovery_period_ms);
    append_tag(&out, shared_secret);
    return out;
}

bool decode_endpoint_discovery_reply(const ByteBuffer& bytes,
                                     const std::string& shared_secret,
                                     uint64_t* nonce,
                                     EndpointAdvertisement* out,
                                     std::string* error) {
    if (nonce == nullptr || out == nullptr || bytes.size() < 36U || bytes.size() > 128U ||
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
        !take_text(bytes, &offset, &decoded.node_name) ||
        !take_u32(bytes, &offset, &decoded.agent_id) ||
        !take_u16(bytes, &offset, &decoded.tcp_listen_port) ||
        !take_u16(bytes, &offset, &decoded.udp_bind_port) ||
        !take_u16(bytes, &offset, &capability_bits) ||
        !take_u64(bytes, &offset, &decoded.sequence) ||
        !take_u32(bytes, &offset, &decoded.discovery_period_ms) || offset + 8U != bytes.size() ||
        *nonce == 0 || !validate_endpoint_id(decoded.endpoint_id)) {
        set_error(error, "invalid discovery reply fields");
        return false;
    }
    decoded.agent_type = "uav";
    decoded.role = "vehicle";
    decoded.protocol_version = "0.1.0";
    decoded.capabilities = capabilities_from_mask(capability_bits);
    decoded.display_name = make_endpoint_display_name(
        decoded.display_name_prefix, decoded.agent_id, decoded.endpoint_id);
    *out = std::move(decoded);
    return true;
}

}  // namespace yunlink
