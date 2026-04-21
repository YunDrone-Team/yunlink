/**
 * @file include/sunraycom/transport/tcp_client_pool.hpp
 * @brief TCP 客户端池组件定义。
 */

#ifndef SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP
#define SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP

#include <memory>
#include <string>

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/core/runtime_config.hpp"

namespace sunraycom {

class TcpClientPool {
  public:
    TcpClientPool();
    ~TcpClientPool();

    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    void stop();

    ErrorCode
    connect_peer(const std::string& ip, uint16_t port, std::string* out_peer_id = nullptr);
    void close_peer(const std::string& peer_id);

    int send(const std::string& peer_id, const ByteBuffer& bytes);
    int send_envelope(const std::string& peer_id, const SecureEnvelope& envelope);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    struct PeerConn;
    void peer_loop(const std::shared_ptr<PeerConn>& conn);
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_TRANSPORT_TCP_CLIENT_POOL_HPP
