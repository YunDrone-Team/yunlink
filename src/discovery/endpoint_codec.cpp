/**
 * @file src/discovery/endpoint_codec.cpp
 * @brief Endpoint discovery payload encoding and decoding.
 */

#include "yunlink/discovery/endpoint_discovery.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <unordered_map>

namespace yunlink {
namespace {

std::string trim_copy(const std::string& value) {
    std::size_t start = 0;
    while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0) {
        ++start;
    }

    std::size_t end = value.size();
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0) {
        --end;
    }
    return value.substr(start, end - start);
}

std::vector<std::string> split_csv(const std::string& value) {
    std::vector<std::string> out;
    std::stringstream stream(value);
    std::string item;
    while (std::getline(stream, item, ',')) {
        item = trim_copy(item);
        if (!item.empty()) {
            out.push_back(item);
        }
    }
    return out;
}

std::string join_csv(const std::vector<std::string>& items) {
    std::ostringstream oss;
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (index != 0U) {
            oss << ',';
        }
        oss << items[index];
    }
    return oss.str();
}

bool parse_u32(const std::string& value, uint32_t* out) {
    if (out == nullptr) {
        return false;
    }
    try {
        const unsigned long parsed = std::stoul(trim_copy(value));
        if (parsed > static_cast<unsigned long>(UINT32_MAX)) {
            return false;
        }
        *out = static_cast<uint32_t>(parsed);
        return true;
    } catch (...) {
        return false;
    }
}

bool parse_u16(const std::string& value, uint16_t* out) {
    uint32_t parsed = 0;
    if (!parse_u32(value, &parsed) || parsed > static_cast<uint32_t>(UINT16_MAX)) {
        return false;
    }
    *out = static_cast<uint16_t>(parsed);
    return true;
}

bool parse_u64(const std::string& value, uint64_t* out) {
    if (out == nullptr) {
        return false;
    }
    try {
        *out = static_cast<uint64_t>(std::stoull(trim_copy(value)));
        return true;
    } catch (...) {
        return false;
    }
}

void set_error(std::string* error, const std::string& value) {
    if (error != nullptr) {
        *error = value;
    }
}

bool field_present(const std::unordered_map<std::string, std::string>& fields,
                   const std::string& key) {
    return fields.find(key) != fields.end();
}

bool require_field(const std::unordered_map<std::string, std::string>& fields,
                   const std::string& key,
                   std::string* error) {
    if (field_present(fields, key)) {
        return true;
    }
    set_error(error, "missing field: " + key);
    return false;
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
    if ((mask & 1U) != 0U) result.push_back("state");
    if ((mask & 2U) != 0U) result.push_back("commands");
    if ((mask & 4U) != 0U) result.push_back("system_service");
    if ((mask & 8U) != 0U) result.push_back("config-resource-v1");
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

std::string make_endpoint_display_name(const std::string& prefix,
                                       uint32_t agent_id,
                                       const std::string& endpoint_id) {
    const std::string safe_prefix =
        prefix.empty() ? std::string(kDefaultEndpointNamePrefix) : prefix;
    return safe_prefix + std::to_string(agent_id) + "_" + endpoint_id;
}

bool validate_endpoint_id(const std::string& endpoint_id) {
    if (endpoint_id.size() != 5U) {
        return false;
    }
    return std::all_of(endpoint_id.begin(), endpoint_id.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0 || (ch >= 'a' && ch <= 'z');
    });
}

ByteBuffer encode_endpoint_advertisement(const EndpointAdvertisement& advertisement) {
    EndpointAdvertisement normalized = advertisement;
    if (normalized.display_name_prefix.empty()) {
        normalized.display_name_prefix = kDefaultEndpointNamePrefix;
    }
    normalized.display_name = make_endpoint_display_name(
        normalized.display_name_prefix, normalized.agent_id, normalized.endpoint_id);

    std::ostringstream oss;
    oss << kEndpointDiscoveryMagic << '\n';
    oss << "endpoint_id=" << normalized.endpoint_id << '\n';
    oss << "display_name=" << normalized.display_name << '\n';
    oss << "display_name_prefix=" << normalized.display_name_prefix << '\n';
    oss << "agent_type=" << normalized.agent_type << '\n';
    oss << "agent_id=" << normalized.agent_id << '\n';
    oss << "role=" << normalized.role << '\n';
    oss << "node_name=" << normalized.node_name << '\n';
    oss << "tcp_listen_port=" << normalized.tcp_listen_port << '\n';
    oss << "udp_bind_port=" << normalized.udp_bind_port << '\n';
    oss << "protocol_version=" << normalized.protocol_version << '\n';
    oss << "capabilities=" << join_csv(normalized.capabilities) << '\n';
    oss << "started_at_ms=" << normalized.started_at_ms << '\n';
    oss << "sequence=" << normalized.sequence << '\n';
    oss << "discovery_period_ms=" << normalized.discovery_period_ms << '\n';

    const std::string text = oss.str();
    return ByteBuffer(text.begin(), text.end());
}

bool decode_endpoint_advertisement(const ByteBuffer& bytes,
                                   EndpointAdvertisement* out,
                                   std::string* error) {
    const std::string text(bytes.begin(), bytes.end());
    return decode_endpoint_advertisement_text(text, out, error);
}

bool decode_endpoint_advertisement_text(const std::string& text,
                                        EndpointAdvertisement* out,
                                        std::string* error) {
    if (out == nullptr) {
        set_error(error, "output is null");
        return false;
    }

    std::stringstream stream(text);
    std::string line;
    if (!std::getline(stream, line)) {
        set_error(error, "empty payload");
        return false;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    if (line != kEndpointDiscoveryMagic) {
        set_error(error, "invalid magic");
        return false;
    }

    std::unordered_map<std::string, std::string> fields;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }
        const std::size_t pos = line.find('=');
        if (pos == std::string::npos || pos == 0U) {
            continue;
        }
        fields[trim_copy(line.substr(0, pos))] = trim_copy(line.substr(pos + 1));
    }

    if (!require_field(fields, "endpoint_id", error) || !require_field(fields, "agent_id", error) ||
        !require_field(fields, "agent_type", error) || !require_field(fields, "role", error) ||
        !require_field(fields, "node_name", error) ||
        !require_field(fields, "tcp_listen_port", error) ||
        !require_field(fields, "udp_bind_port", error) ||
        !require_field(fields, "protocol_version", error) ||
        !require_field(fields, "sequence", error)) {
        return false;
    }

    EndpointAdvertisement decoded{};
    decoded.endpoint_id = fields["endpoint_id"];
    if (!validate_endpoint_id(decoded.endpoint_id)) {
        set_error(error, "invalid endpoint_id");
        return false;
    }

    decoded.display_name_prefix = fields.count("display_name_prefix") != 0U
                                      ? fields["display_name_prefix"]
                                      : std::string(kDefaultEndpointNamePrefix);
    decoded.agent_type = fields["agent_type"];
    decoded.role = fields["role"];
    decoded.node_name = fields["node_name"];
    decoded.protocol_version = fields["protocol_version"];
    decoded.capabilities = fields.count("capabilities") != 0U ? split_csv(fields["capabilities"])
                                                              : std::vector<std::string>{};

    if (!parse_u32(fields["agent_id"], &decoded.agent_id) ||
        !parse_u16(fields["tcp_listen_port"], &decoded.tcp_listen_port) ||
        !parse_u16(fields["udp_bind_port"], &decoded.udp_bind_port) ||
        !parse_u64(fields["sequence"], &decoded.sequence)) {
        set_error(error, "invalid numeric field");
        return false;
    }

    if (fields.count("started_at_ms") != 0U &&
        !parse_u64(fields["started_at_ms"], &decoded.started_at_ms)) {
        set_error(error, "invalid started_at_ms");
        return false;
    }
    if (fields.count("discovery_period_ms") != 0U &&
        !parse_u32(fields["discovery_period_ms"], &decoded.discovery_period_ms)) {
        set_error(error, "invalid discovery_period_ms");
        return false;
    }

    decoded.display_name = make_endpoint_display_name(
        decoded.display_name_prefix, decoded.agent_id, decoded.endpoint_id);
    *out = decoded;
    return true;
}

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
    if (!take_u64(bytes, &offset, &out->nonce) || !take_u16(bytes, &offset, &out->response_window_ms) ||
        out->nonce == 0 || out->response_window_ms < 50U || out->response_window_ms > 2000U) {
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
        !take_text(bytes, &offset, &decoded.node_name) || !take_u32(bytes, &offset, &decoded.agent_id) ||
        !take_u16(bytes, &offset, &decoded.tcp_listen_port) ||
        !take_u16(bytes, &offset, &decoded.udp_bind_port) || !take_u16(bytes, &offset, &capability_bits) ||
        !take_u64(bytes, &offset, &decoded.sequence) ||
        !take_u32(bytes, &offset, &decoded.discovery_period_ms) || offset + 8U != bytes.size() || *nonce == 0 ||
        !validate_endpoint_id(decoded.endpoint_id)) {
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
