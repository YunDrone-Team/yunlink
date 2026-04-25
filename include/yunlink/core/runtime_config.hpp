/**
 * @file include/yunlink/core/runtime_config.hpp
 * @brief 运行时配置定义。
 */

#ifndef YUNLINK_CORE_RUNTIME_CONFIG_HPP
#define YUNLINK_CORE_RUNTIME_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include "yunlink/core/types.hpp"

namespace yunlink {

enum class CommandHandlingMode : uint8_t {
    kAutoResult = 1,
    kExternalHandler = 2,
};

constexpr uint32_t kCapabilityExternalExecutor = 1u << 0;
constexpr uint32_t kCapabilityBulkChannel = 1u << 1;
constexpr uint32_t kCapabilityPlanningBridge = 1u << 2;
constexpr uint32_t kCapabilitySwarmGroup = 1u << 3;
constexpr uint32_t kCapabilitySecurityTags = 1u << 4;

struct RuntimeConfig {
    uint16_t udp_bind_port = 9696;
    uint16_t udp_target_port = 9898;
    uint16_t tcp_listen_port = 9696;
    int connect_timeout_ms = 5000;
    int io_poll_interval_ms = 5;
    size_t max_buffer_bytes_per_peer = 1 << 20;
    std::string multicast_group = "224.1.1.1";

    EndpointIdentity self_identity{
        AgentType::kGroundStation,
        0,
        EndpointRole::kObserver,
    };
    uint32_t capability_flags = 0;
    std::string shared_secret = "yunlink-default-secret";
    CommandHandlingMode command_handling_mode = CommandHandlingMode::kAutoResult;
    uint32_t trajectory_chunk_timeout_ms = 1000;
    uint32_t security_key_epoch = 1;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_RUNTIME_CONFIG_HPP
