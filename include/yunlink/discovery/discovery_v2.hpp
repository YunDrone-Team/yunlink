/**
 * @file include/yunlink/discovery/discovery_v2.hpp
 * @brief Provider-neutral authenticated UDP discovery for YunLink Wire v2.
 */

#ifndef YUNLINK_DISCOVERY_DISCOVERY_V2_HPP
#define YUNLINK_DISCOVERY_DISCOVERY_V2_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "yunlink/core/wire_v2.hpp"

namespace yunlink::v2 {

constexpr uint16_t kDefaultDiscoveryPort = 9697;
// Discovery packets are independently versioned from the Wire schema. Keep v2
// for legacy clients; v3 adds the authoritative per-entity Agent ID in replies.
constexpr uint8_t kDiscoveryFormatV2 = 2;
constexpr uint8_t kDiscoveryFormatV3 = 3;

struct DiscoveryQuery {
    uint64_t nonce = 0;
    uint16_t response_window_ms = 250;
    uint8_t format_version = kDiscoveryFormatV2;
};

struct DiscoveryEntitySummary {
    std::string entity_uid;
    std::string kind;
    std::string display_name;
    Availability availability = Availability::kUnknown;
    // Zero means an older discovery-v2 reply did not carry this field.
    uint32_t agent_id = 0;
};

struct DiscoveryAdvertisement {
    std::string endpoint_uid;
    std::string display_name = "yunlink-endpoint";
    uint16_t tcp_listen_port = 9696;
    std::vector<std::string> capabilities;
    std::vector<ProfileDescriptor> profiles;
    std::vector<DiscoveryEntitySummary> entities;
    uint64_t started_at_ms = 0;
    uint64_t sequence = 0;
};

Bytes encode_discovery_query(const DiscoveryQuery& query, const std::string& shared_secret);
bool decode_discovery_query(const Bytes& bytes,
                            const std::string& shared_secret,
                            DiscoveryQuery* query);
Bytes encode_discovery_reply(const DiscoveryQuery& query,
                             const DiscoveryAdvertisement& advertisement,
                             const std::string& shared_secret);
bool decode_discovery_reply(const Bytes& bytes,
                            const std::string& shared_secret,
                            uint64_t expected_nonce,
                            DiscoveryAdvertisement* advertisement);

class DiscoveryAdvertiser {
  public:
    struct Impl;

    DiscoveryAdvertiser();
    ~DiscoveryAdvertiser();
    DiscoveryAdvertiser(const DiscoveryAdvertiser&) = delete;
    DiscoveryAdvertiser& operator=(const DiscoveryAdvertiser&) = delete;

    ErrorCode
    start(uint16_t bind_port, DiscoveryAdvertisement advertisement, std::string shared_secret);
    void stop();
    bool running() const;
    void set_advertisement(DiscoveryAdvertisement advertisement);

  private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace yunlink::v2

#endif  // YUNLINK_DISCOVERY_DISCOVERY_V2_HPP
