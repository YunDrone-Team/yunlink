/**
 * @file include/sunraycom/core/runtime_config.hpp
 * @brief 运行时配置定义。
 */

#ifndef SUNRAYCOM_CORE_RUNTIME_CONFIG_HPP
#define SUNRAYCOM_CORE_RUNTIME_CONFIG_HPP

#include <cstddef>
#include <cstdint>
#include <string>

#include "sunraycom/core/types.hpp"

namespace sunraycom {

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
    std::string shared_secret = "sunray-default-secret";
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_RUNTIME_CONFIG_HPP
