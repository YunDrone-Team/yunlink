/**
 * @file include/yunlink/transport/tcp_server.hpp
 * @brief TCP 服务端组件定义。
 */

#ifndef YUNLINK_TRANSPORT_TCP_SERVER_HPP
#define YUNLINK_TRANSPORT_TCP_SERVER_HPP

#include <memory>
#include <string>

#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/protocol_codec.hpp"
#include "yunlink/core/runtime_config.hpp"

namespace yunlink {

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

}  // namespace yunlink

#endif  // YUNLINK_TRANSPORT_TCP_SERVER_HPP
