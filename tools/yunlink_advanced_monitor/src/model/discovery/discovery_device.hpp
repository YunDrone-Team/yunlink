#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_DISCOVERY_DISCOVERY_DEVICE_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_DISCOVERY_DISCOVERY_DEVICE_HPP

#include <cstdint>
#include <string>
#include <vector>

struct DiscoveryDevice {
    std::string dedupe_key;
    std::string display_name;
    std::string endpoint_id;
    std::string display_name_prefix;
    std::string agent_type;
    uint32_t agent_id{0};
    std::string role;
    std::string source_ip;
    uint16_t source_port{0};
    uint16_t tcp_listen_port{0};
    uint16_t udp_bind_port{0};
    std::string node_name;
    std::string protocol_version;
    std::vector<std::string> capabilities;
    uint64_t last_seen_ms{0};
    uint64_t last_query_reply_ms{0};
    uint64_t started_at_ms{0};
    uint64_t sequence{0};
    uint32_t discovery_period_ms{1000};
    bool stale{false};
    bool selected{false};
};

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_DISCOVERY_DISCOVERY_DEVICE_HPP
