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
constexpr const char* kDefaultEndpointNamePrefix = "yundrone_uav";
constexpr uint16_t kDefaultEndpointDiscoveryPort = 9966;

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
};

struct EndpointAdvertisementPacket {
    EndpointAdvertisement advertisement;
    std::string source_ip;
    uint16_t source_port{0};
    uint64_t received_at_ms{0};
};

struct EndpointDiscoveryConfig {
    uint16_t discovery_port{kDefaultEndpointDiscoveryPort};
    std::string target_ip{"255.255.255.255"};
    int io_poll_interval_ms{10};
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

class EndpointAdvertiser {
  public:
    EndpointAdvertiser();
    ~EndpointAdvertiser();

    ErrorCode start(const EndpointDiscoveryConfig& config);
    void stop();

    bool is_running() const;
    int send(const EndpointAdvertisement& advertisement);
    std::string last_error() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void set_last_error(const std::string& error);
};

class EndpointListener {
  public:
    EndpointListener();
    ~EndpointListener();

    ErrorCode start(const EndpointDiscoveryConfig& config);
    void stop();

    bool is_running() const;
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
