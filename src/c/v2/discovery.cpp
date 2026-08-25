#include "yunlink/c/yunlink_v2.h"

#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <string>

#include "yunlink/core/wire_v2.hpp"
#include "yunlink/discovery/discovery_v2.hpp"

struct yunlink_v2_discovery_advertisement {
    yunlink::v2::DiscoveryAdvertisement value;
};

namespace {

std::string copy(yunlink_v2_string_view_t value) {
    return value.data == nullptr ? std::string{} : std::string(value.data, value.len);
}

yunlink_v2_string_view_t view(const std::string& value) {
    return {value.data(), value.size()};
}

yunlink_v2_string_view_t empty_view() {
    return {nullptr, 0};
}

template <typename Map>
bool attribute_at(const Map& values, size_t index, yunlink_v2_key_value_view_t* out) {
    if (out == nullptr || index >= values.size()) {
        return false;
    }
    auto item = values.begin();
    std::advance(item, static_cast<typename Map::difference_type>(index));
    out->key = view(item->first);
    out->value = view(item->second);
    return true;
}

}  // namespace

extern "C" {

uint16_t yunlink_v2_discovery_encode_query(uint64_t nonce,
                                           uint16_t response_window_ms,
                                           uint8_t format_version,
                                           yunlink_v2_string_view_t shared_secret,
                                           uint8_t* out_bytes,
                                           size_t out_capacity,
                                           size_t* out_size) {
    if (out_size == nullptr) {
        return static_cast<uint16_t>(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    const auto encoded = yunlink::v2::encode_discovery_query(
        {nonce, response_window_ms, format_version}, copy(shared_secret));
    if (encoded.empty()) {
        *out_size = 0;
        return static_cast<uint16_t>(yunlink::v2::ErrorCode::kEncodeError);
    }
    *out_size = encoded.size();
    if (out_bytes == nullptr || out_capacity < encoded.size()) {
        return static_cast<uint16_t>(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    std::memcpy(out_bytes, encoded.data(), encoded.size());
    return static_cast<uint16_t>(yunlink::v2::ErrorCode::kOk);
}

yunlink_v2_discovery_advertisement_t*
yunlink_v2_discovery_decode_reply(yunlink_v2_bytes_view_t bytes,
                                  yunlink_v2_string_view_t shared_secret,
                                  uint64_t expected_nonce) {
    if (bytes.data == nullptr || bytes.len == 0) {
        return nullptr;
    }
    auto decoded = std::make_unique<yunlink_v2_discovery_advertisement>();
    const yunlink::v2::Bytes source(bytes.data, bytes.data + bytes.len);
    if (!yunlink::v2::decode_discovery_reply(
            source, copy(shared_secret), expected_nonce, &decoded->value)) {
        return nullptr;
    }
    return decoded.release();
}

void yunlink_v2_discovery_advertisement_destroy(
    yunlink_v2_discovery_advertisement_t* advertisement) {
    delete advertisement;
}

yunlink_v2_string_view_t
yunlink_v2_discovery_endpoint_uid(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? empty_view() : view(advertisement->value.endpoint_uid);
}

yunlink_v2_string_view_t
yunlink_v2_discovery_display_name(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? empty_view() : view(advertisement->value.display_name);
}

uint16_t yunlink_v2_discovery_tcp_port(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.tcp_listen_port;
}

uint64_t
yunlink_v2_discovery_started_at_ms(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.started_at_ms;
}

uint64_t yunlink_v2_discovery_sequence(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.sequence;
}

size_t
yunlink_v2_discovery_capability_count(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.capabilities.size();
}

yunlink_v2_string_view_t
yunlink_v2_discovery_capability_at(const yunlink_v2_discovery_advertisement_t* advertisement,
                                   size_t index) {
    if (advertisement == nullptr || index >= advertisement->value.capabilities.size()) {
        return empty_view();
    }
    return view(advertisement->value.capabilities[index]);
}

size_t
yunlink_v2_discovery_profile_count(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.profiles.size();
}

uint8_t yunlink_v2_discovery_profile_at(const yunlink_v2_discovery_advertisement_t* advertisement,
                                        size_t index,
                                        yunlink_v2_profile_view_t* out_profile) {
    if (advertisement == nullptr || out_profile == nullptr ||
        index >= advertisement->value.profiles.size()) {
        return 0;
    }
    const auto& profile = advertisement->value.profiles[index];
    *out_profile = {
        view(profile.profile_id), profile.major, profile.minor, view(profile.schema_digest)};
    return 1;
}

size_t
yunlink_v2_discovery_attribute_count(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.attributes.size();
}

uint8_t yunlink_v2_discovery_attribute_at(const yunlink_v2_discovery_advertisement_t* advertisement,
                                          size_t index,
                                          yunlink_v2_key_value_view_t* out_attribute) {
    return advertisement != nullptr &&
           attribute_at(advertisement->value.attributes, index, out_attribute);
}

size_t
yunlink_v2_discovery_entity_count(const yunlink_v2_discovery_advertisement_t* advertisement) {
    return advertisement == nullptr ? 0 : advertisement->value.entities.size();
}

uint8_t yunlink_v2_discovery_entity_at(const yunlink_v2_discovery_advertisement_t* advertisement,
                                       size_t index,
                                       yunlink_v2_discovery_entity_view_t* out_entity) {
    if (advertisement == nullptr || out_entity == nullptr ||
        index >= advertisement->value.entities.size()) {
        return 0;
    }
    const auto& entity = advertisement->value.entities[index];
    *out_entity = {view(entity.entity_uid),
                   view(entity.kind),
                   view(entity.display_name),
                   static_cast<uint8_t>(entity.availability),
                   entity.agent_id};
    return 1;
}

size_t yunlink_v2_discovery_entity_attribute_count(
    const yunlink_v2_discovery_advertisement_t* advertisement,
    size_t entity_index) {
    if (advertisement == nullptr || entity_index >= advertisement->value.entities.size()) {
        return 0;
    }
    return advertisement->value.entities[entity_index].attributes.size();
}

uint8_t
yunlink_v2_discovery_entity_attribute_at(const yunlink_v2_discovery_advertisement_t* advertisement,
                                         size_t entity_index,
                                         size_t attribute_index,
                                         yunlink_v2_key_value_view_t* out_attribute) {
    return advertisement != nullptr && entity_index < advertisement->value.entities.size() &&
           attribute_at(advertisement->value.entities[entity_index].attributes,
                        attribute_index,
                        out_attribute);
}

}  // extern "C"
