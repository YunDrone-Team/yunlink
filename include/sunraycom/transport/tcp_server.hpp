/**
 * @file include/sunraycom/transport/tcp_server.hpp
 * @brief TCP 服务端组件定义。
 */

#ifndef SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP
#define SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP

#include <memory>
#include <string>

#include "sunraycom/core/event_bus.hpp"
#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/core/runtime_config.hpp"

namespace sunraycom {

class TcpServer {
  public:
    TcpServer();
    ~TcpServer();

    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    void stop();

    int send(const std::string& peer_id, const ByteBuffer& bytes);
    int send_envelope(const std::string& peer_id, const SecureEnvelope& envelope);
    void broadcast(const ByteBuffer& bytes);
    void broadcast_envelope(const SecureEnvelope& envelope);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    struct ClientConn;
    void accept_loop();
    void client_loop(const std::shared_ptr<ClientConn>& conn);
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_TRANSPORT_TCP_SERVER_HPP
