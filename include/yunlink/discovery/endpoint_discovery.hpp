/**
 * @file include/yunlink/discovery/endpoint_discovery.hpp
 * @brief ROS-independent endpoint discovery primitives.
 */

#ifndef YUNLINK_DISCOVERY_ENDPOINT_DISCOVERY_HPP
#define YUNLINK_DISCOVERY_ENDPOINT_DISCOVERY_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "yunlink/core/types.hpp"

namespace yunlink {

constexpr const char* kEndpointDiscoveryMagic = "YUNLINK_ENDPOINT_DISCOVERY_V1";
constexpr const char* kEndpointDiscoveryQueryMagic = "YLQ1";
constexpr const char* kEndpointDiscoveryReplyMagic = "YLR1";
constexpr const char* kDefaultEndpointNamePrefix = "SURY-uav";
constexpr uint16_t kDefaultEndpointDiscoveryPort = 9966;
constexpr std::size_t kMaxDiscoveryManagedEntitySummaries = 8U;

/// A bounded, display-safe summary of one entity managed by a discovered endpoint.
/// It is discovery metadata only: an authenticated session remains authoritative.
struct EndpointManagedEntitySummary {
    std::string entity_uid;
    std::string agent_type{"uav"};
    uint32_t agent_id{0};
    std::string display_name;
    std::string node_name;
};

struct EndpointAdvertisement {
    std::string endpoint_id;
    std::string display_name;
    std::string display_name_prefix{kDefaultEndpointNamePrefix};
    std::string agent_type{"uav"};
    uint32_t agent_id{0};
    std::string role{"vehicle"};
    std::string node_name;
    uint16_t tcp_listen_port{0};
    uint16_t udp_bind_port{0};
    std::string protocol_version{"0.1.0"};
    std::vector<std::string> capabilities;
    uint64_t started_at_ms{0};
    uint64_t sequence{0};
    uint32_t discovery_period_ms{1000};
    /// False for legacy endpoint announcements that do not publish an entity directory summary.
    bool managed_entity_count_known{false};
    uint16_t managed_entity_count{0};
    /// May be truncated to kMaxDiscoveryManagedEntitySummaries while count retains the real total.
    std::vector<EndpointManagedEntitySummary> managed_entities;
};

struct EndpointAdvertisementPacket {
    EndpointAdvertisement advertisement;
    std::string source_ip;
    uint16_t source_port{0};
    uint64_t received_at_ms{0};
    bool is_query_reply{false};
    uint64_t reply_nonce{0};
};

struct EndpointDiscoveryConfig {
    uint16_t discovery_port{kDefaultEndpointDiscoveryPort};
    std::string target_ip{"255.255.255.255"};
    int io_poll_interval_ms{10};
    std::string shared_secret{"yunlink-default-secret"};
    uint16_t query_response_window_ms{1000};
    uint16_t query_rate_limit_per_sec{6};
};

struct EndpointDiscoveryQuery {
    uint64_t nonce{0};
    uint16_t response_window_ms{1000};
};

std::string make_endpoint_display_name(const std::string& prefix,
                                       uint32_t agent_id,
                                       const std::string& endpoint_id);
bool validate_endpoint_id(const std::string& endpoint_id);
ByteBuffer encode_endpoint_advertisement(const EndpointAdvertisement& advertisement);
bool decode_endpoint_advertisement(const ByteBuffer& bytes,
                                   EndpointAdvertisement* out,
                                   std::string* error = nullptr);
bool decode_endpoint_advertisement_text(const std::string& text,
                                        EndpointAdvertisement* out,
                                        std::string* error = nullptr);
ByteBuffer encode_endpoint_discovery_query(const EndpointDiscoveryQuery& query,
                                           const std::string& shared_secret);
bool decode_endpoint_discovery_query(const ByteBuffer& bytes,
                                     const std::string& shared_secret,
                                     EndpointDiscoveryQuery* out,
                                     std::string* error = nullptr);
ByteBuffer encode_endpoint_discovery_reply(uint64_t nonce,
                                           const EndpointAdvertisement& advertisement,
                                           const std::string& shared_secret);
bool decode_endpoint_discovery_reply(const ByteBuffer& bytes,
                                     const std::string& shared_secret,
                                     uint64_t* nonce,
                                     EndpointAdvertisement* out,
                                     std::string* error = nullptr);

class EndpointAdvertiser {
  public:
    EndpointAdvertiser();
    ~EndpointAdvertiser();

    ErrorCode start(const EndpointDiscoveryConfig& config);
    void stop();

    bool is_running() const;
    void set_advertisement(const EndpointAdvertisement& advertisement);
    int send(const EndpointAdvertisement& advertisement);
    std::string last_error() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void recv_loop();
    void set_last_error(const std::string& error);
};

class EndpointListener {
  public:
    EndpointListener();
    ~EndpointListener();

    ErrorCode start(const EndpointDiscoveryConfig& config);
    void stop();

    bool is_running() const;
    ErrorCode send_query(uint64_t nonce, uint16_t response_window_ms);
    size_t drain(std::vector<EndpointAdvertisementPacket>* out);
    std::string last_error() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void recv_loop();
    void set_last_error(const std::string& error);
};

}  // namespace yunlink

#endif  // YUNLINK_DISCOVERY_ENDPOINT_DISCOVERY_HPP
