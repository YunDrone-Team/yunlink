#include "yunlink/discovery/discovery_v2.hpp"

#include <algorithm>
#include <array>
namespace yunlink::v2 {
namespace {

constexpr std::array<uint8_t, 4> kQueryMagicV2{{'Y', 'L', 'Q', '2'}};
constexpr std::array<uint8_t, 4> kQueryMagicV3{{'Y', 'L', 'Q', '3'}};
constexpr std::array<uint8_t, 4> kQueryMagicV4{{'Y', 'L', 'Q', '4'}};
constexpr std::array<uint8_t, 4> kReplyMagicV2{{'Y', 'L', 'R', '2'}};
constexpr std::array<uint8_t, 4> kReplyMagicV3{{'Y', 'L', 'R', '3'}};
constexpr std::array<uint8_t, 4> kReplyMagicV4{{'Y', 'L', 'R', '4'}};
constexpr size_t kMaxDiscoveryBytes = 8192;
constexpr size_t kMaxAttributeCount = 32;
constexpr size_t kMaxAttributeKeyBytes = 64;
constexpr size_t kMaxAttributeValueBytes = 512;
constexpr uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;

uint64_t auth_tag(const std::string& secret, const uint8_t* data, size_t size) {
    uint64_t value = kFnvOffset;
    for (const unsigned char byte : secret) {
        value = (value ^ byte) * kFnvPrime;
    }
    for (size_t index = 0; index < size; ++index) {
        value = (value ^ data[index]) * kFnvPrime;
    }
    return value;
}

class Writer {
  public:
    void bytes(const uint8_t* data, size_t size) {
        value_.insert(value_.end(), data, data + size);
    }
    void u8(uint8_t value) {
        value_.push_back(value);
    }
    void u16(uint16_t value) {
        value_.push_back(static_cast<uint8_t>(value >> 8U));
        value_.push_back(static_cast<uint8_t>(value));
    }
    void u32(uint32_t value) {
        for (int shift = 24; shift >= 0; shift -= 8) {
            value_.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
    void u64(uint64_t value) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            value_.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
    bool text(const std::string& value) {
        if (value.size() > UINT16_MAX) {
            valid_ = false;
            return false;
        }
        u16(static_cast<uint16_t>(value.size()));
        value_.insert(value_.end(), value.begin(), value.end());
        return true;
    }
    bool attributes(const std::map<std::string, std::string>& values) {
        if (values.size() > kMaxAttributeCount) {
            valid_ = false;
            return false;
        }
        u8(static_cast<uint8_t>(values.size()));
        for (const auto& [key, value] : values) {
            if (key.empty() || key.size() > kMaxAttributeKeyBytes ||
                value.size() > kMaxAttributeValueBytes) {
                valid_ = false;
                return false;
            }
            text(key);
            text(value);
        }
        return valid_;
    }
    Bytes finish(const std::string& secret) {
        if (!valid_ || value_.size() + sizeof(uint64_t) > kMaxDiscoveryBytes) {
            return {};
        }
        u64(auth_tag(secret, value_.data(), value_.size()));
        return std::move(value_);
    }

  private:
    Bytes value_;
    bool valid_ = true;
};

class Reader {
  public:
    Reader(const Bytes& value, size_t payload_size) : value_(value), payload_size_(payload_size) {}
    bool magic(const std::array<uint8_t, 4>& expected) {
        if (cursor_ + expected.size() > payload_size_ ||
            !std::equal(expected.begin(), expected.end(), value_.data() + cursor_)) {
            return false;
        }
        cursor_ += expected.size();
        return true;
    }
    bool u8(uint8_t* value) {
        if (value == nullptr || cursor_ + 1 > payload_size_) {
            return false;
        }
        *value = value_[cursor_++];
        return true;
    }
    bool u16(uint16_t* value) {
        if (value == nullptr || cursor_ + 2 > payload_size_) {
            return false;
        }
        *value = static_cast<uint16_t>(value_[cursor_] << 8U) |
                 static_cast<uint16_t>(value_[cursor_ + 1]);
        cursor_ += 2;
        return true;
    }
    bool u32(uint32_t* value) {
        if (value == nullptr || cursor_ + 4 > payload_size_) {
            return false;
        }
        *value = 0;
        for (int shift = 24; shift >= 0; shift -= 8) {
            *value |= static_cast<uint32_t>(value_[cursor_++]) << shift;
        }
        return true;
    }
    bool u64(uint64_t* value) {
        if (value == nullptr || cursor_ + 8 > payload_size_) {
            return false;
        }
        *value = 0;
        for (int shift = 56; shift >= 0; shift -= 8) {
            *value |= static_cast<uint64_t>(value_[cursor_++]) << shift;
        }
        return true;
    }
    bool text(std::string* value) {
        uint16_t size = 0;
        if (value == nullptr || !u16(&size) || cursor_ + size > payload_size_) {
            return false;
        }
        value->assign(reinterpret_cast<const char*>(value_.data() + cursor_), size);
        cursor_ += size;
        return true;
    }
    bool attributes(std::map<std::string, std::string>* values) {
        uint8_t count = 0;
        if (values == nullptr || !u8(&count) || count > kMaxAttributeCount) {
            return false;
        }
        values->clear();
        for (uint8_t index = 0; index < count; ++index) {
            std::string key;
            std::string value;
            if (!text(&key) || key.empty() || key.size() > kMaxAttributeKeyBytes || !text(&value) ||
                value.size() > kMaxAttributeValueBytes ||
                !values->emplace(std::move(key), std::move(value)).second) {
                return false;
            }
        }
        return true;
    }
    bool done() const {
        return cursor_ == payload_size_;
    }

  private:
    const Bytes& value_;
    size_t payload_size_ = 0;
    size_t cursor_ = 0;
};

bool authenticated(const Bytes& bytes, const std::string& secret) {
    if (bytes.size() < sizeof(uint64_t)) {
        return false;
    }
    const size_t payload_size = bytes.size() - sizeof(uint64_t);
    uint64_t supplied = 0;
    for (size_t index = payload_size; index < bytes.size(); ++index) {
        supplied = (supplied << 8U) | bytes[index];
    }
    return supplied == auth_tag(secret, bytes.data(), payload_size);
}

bool valid_advertisement(const DiscoveryAdvertisement& value) {
    if (!valid_uid(value.endpoint_uid) || value.tcp_listen_port == 0 ||
        value.capabilities.size() > UINT8_MAX || value.profiles.size() > UINT8_MAX ||
        value.entities.size() > UINT8_MAX) {
        return false;
    }
    const auto valid_attributes = [](const auto& attributes) {
        return attributes.size() <= kMaxAttributeCount &&
               std::all_of(attributes.begin(), attributes.end(), [](const auto& entry) {
                   return !entry.first.empty() && entry.first.size() <= kMaxAttributeKeyBytes &&
                          entry.second.size() <= kMaxAttributeValueBytes;
               });
    };
    return valid_attributes(value.attributes) &&
           std::all_of(value.profiles.begin(),
                       value.profiles.end(),
                       [](const auto& profile) {
                           return valid_profile_id(profile.profile_id) && profile.major > 0;
                       }) &&
           std::all_of(value.entities.begin(), value.entities.end(), [&](const auto& entity) {
               return valid_uid(entity.entity_uid) && valid_attributes(entity.attributes);
           });
}

}  // namespace

bool discovery_advertisement_is_valid(const DiscoveryAdvertisement& value) {
    return valid_advertisement(value);
}

Bytes encode_discovery_query(const DiscoveryQuery& query, const std::string& shared_secret) {
    if (query.nonce == 0 || query.response_window_ms == 0 ||
        (query.format_version != kDiscoveryFormatV2 && query.format_version != kDiscoveryFormatV3 &&
         query.format_version != kDiscoveryFormatV4)) {
        return {};
    }
    Writer writer;
    const auto& magic = query.format_version == kDiscoveryFormatV4   ? kQueryMagicV4
                        : query.format_version == kDiscoveryFormatV3 ? kQueryMagicV3
                                                                     : kQueryMagicV2;
    writer.bytes(magic.data(), magic.size());
    writer.u64(query.nonce);
    writer.u16(query.response_window_ms);
    return writer.finish(shared_secret);
}

bool decode_discovery_query(const Bytes& bytes,
                            const std::string& shared_secret,
                            DiscoveryQuery* query) {
    if (query == nullptr || bytes.size() != 22 || !authenticated(bytes, shared_secret)) {
        return false;
    }
    Reader reader(bytes, bytes.size() - sizeof(uint64_t));
    DiscoveryQuery parsed;
    if (reader.magic(kQueryMagicV4)) {
        parsed.format_version = kDiscoveryFormatV4;
    } else if (reader.magic(kQueryMagicV3)) {
        parsed.format_version = kDiscoveryFormatV3;
    } else if (reader.magic(kQueryMagicV2)) {
        parsed.format_version = kDiscoveryFormatV2;
    } else {
        return false;
    }
    if (!reader.u64(&parsed.nonce) || !reader.u16(&parsed.response_window_ms) || !reader.done() ||
        parsed.nonce == 0 || parsed.response_window_ms == 0) {
        return false;
    }
    *query = parsed;
    return true;
}

Bytes encode_discovery_reply(const DiscoveryQuery& query,
                             const DiscoveryAdvertisement& advertisement,
                             const std::string& shared_secret) {
    if (query.nonce == 0 || !valid_advertisement(advertisement)) {
        return {};
    }
    Writer writer;
    const bool supports_entity_agent_id = query.format_version >= kDiscoveryFormatV3;
    const bool supports_attributes = query.format_version >= kDiscoveryFormatV4;
    const auto& magic = supports_attributes        ? kReplyMagicV4
                        : supports_entity_agent_id ? kReplyMagicV3
                                                   : kReplyMagicV2;
    writer.bytes(magic.data(), magic.size());
    writer.u64(query.nonce);
    writer.u8(kProtocolMajor);
    writer.u8(kHeaderVersion);
    writer.u16(query.format_version);
    writer.text(advertisement.endpoint_uid);
    writer.text(advertisement.display_name);
    writer.u16(advertisement.tcp_listen_port);
    writer.u8(static_cast<uint8_t>(advertisement.capabilities.size()));
    for (const auto& capability : advertisement.capabilities) {
        writer.text(capability);
    }
    writer.u8(static_cast<uint8_t>(advertisement.profiles.size()));
    for (const auto& profile : advertisement.profiles) {
        writer.text(profile.profile_id);
        writer.u16(profile.major);
        writer.u16(profile.minor);
        writer.text(profile.schema_digest);
    }
    if (supports_attributes) {
        writer.attributes(advertisement.attributes);
    }
    writer.u8(static_cast<uint8_t>(advertisement.entities.size()));
    for (const auto& entity : advertisement.entities) {
        writer.text(entity.entity_uid);
        writer.text(entity.kind);
        writer.text(entity.display_name);
        writer.u8(static_cast<uint8_t>(entity.availability));
        if (supports_entity_agent_id) {
            writer.u32(entity.agent_id);
        }
        if (supports_attributes) {
            writer.attributes(entity.attributes);
        }
    }
    writer.u64(advertisement.started_at_ms);
    writer.u64(advertisement.sequence);
    return writer.finish(shared_secret);
}

bool decode_discovery_reply(const Bytes& bytes,
                            const std::string& shared_secret,
                            uint64_t expected_nonce,
                            DiscoveryAdvertisement* advertisement) {
    if (advertisement == nullptr || bytes.size() > kMaxDiscoveryBytes ||
        !authenticated(bytes, shared_secret)) {
        return false;
    }
    Reader reader(bytes, bytes.size() - sizeof(uint64_t));
    DiscoveryAdvertisement parsed;
    uint64_t nonce = 0;
    uint8_t protocol = 0;
    uint8_t header = 0;
    uint16_t schema = 0;
    uint8_t count = 0;
    uint8_t format_version = kDiscoveryFormatV2;
    if (reader.magic(kReplyMagicV4)) {
        format_version = kDiscoveryFormatV4;
    } else if (reader.magic(kReplyMagicV3)) {
        format_version = kDiscoveryFormatV3;
    } else if (reader.magic(kReplyMagicV2)) {
        format_version = kDiscoveryFormatV2;
    } else {
        return false;
    }
    const bool includes_entity_agent_id = format_version >= kDiscoveryFormatV3;
    const bool includes_attributes = format_version >= kDiscoveryFormatV4;
    if (!reader.u64(&nonce) || nonce != expected_nonce || !reader.u8(&protocol) ||
        protocol != kProtocolMajor || !reader.u8(&header) || header != kHeaderVersion ||
        !reader.u16(&schema) || schema != format_version || !reader.text(&parsed.endpoint_uid) ||
        !reader.text(&parsed.display_name) || !reader.u16(&parsed.tcp_listen_port) ||
        !reader.u8(&count)) {
        return false;
    }
    parsed.capabilities.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        std::string capability;
        if (!reader.text(&capability)) {
            return false;
        }
        parsed.capabilities.push_back(std::move(capability));
    }
    if (!reader.u8(&count)) {
        return false;
    }
    parsed.profiles.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        ProfileDescriptor profile;
        if (!reader.text(&profile.profile_id) || !reader.u16(&profile.major) ||
            !reader.u16(&profile.minor) || !reader.text(&profile.schema_digest)) {
            return false;
        }
        parsed.profiles.push_back(std::move(profile));
    }
    if (includes_attributes && !reader.attributes(&parsed.attributes)) {
        return false;
    }
    if (!reader.u8(&count)) {
        return false;
    }
    parsed.entities.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        DiscoveryEntitySummary entity;
        uint8_t availability = 0;
        if (!reader.text(&entity.entity_uid) || !reader.text(&entity.kind) ||
            !reader.text(&entity.display_name) || !reader.u8(&availability) ||
            availability > static_cast<uint8_t>(Availability::kOffline)) {
            return false;
        }
        if (includes_entity_agent_id && !reader.u32(&entity.agent_id)) {
            return false;
        }
        if (includes_attributes && !reader.attributes(&entity.attributes)) {
            return false;
        }
        entity.availability = static_cast<Availability>(availability);
        parsed.entities.push_back(std::move(entity));
    }
    if (!reader.u64(&parsed.started_at_ms) || !reader.u64(&parsed.sequence) || !reader.done() ||
        !valid_advertisement(parsed)) {
        return false;
    }
    *advertisement = std::move(parsed);
    return true;
}

}  // namespace yunlink::v2
