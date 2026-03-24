/**
 * @file include/sunraycom/sunraycom.hpp
 * @brief SunrayComLib C++ 统一入口头文件。
 */

#ifndef SUNRAYCOM_SUNRAYCOM_HPP
#define SUNRAYCOM_SUNRAYCOM_HPP

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/frame_stream_parser.hpp"
#include "sunraycom/core/message_ids.hpp"
#include "sunraycom/core/payload_mapper.hpp"
#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/core/runtime_config.hpp"
#include "sunraycom/compat/legacy_adapter.hpp"
#include "sunraycom/compat/legacy_types.hpp"
#include "sunraycom/runtime/runtime.hpp"
#include "sunraycom/transport/tcp_client_pool.hpp"
#include "sunraycom/transport/tcp_server.hpp"
#include "sunraycom/transport/udp_transport.hpp"

/**
 * @brief SunrayComLib 顶层命名空间。
 */
namespace sunraycom {}

#endif  // SUNRAYCOM_SUNRAYCOM_HPP
