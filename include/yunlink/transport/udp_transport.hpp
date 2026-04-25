/**
 * @file include/yunlink/transport/udp_transport.hpp
 * @brief UDP 传输组件定义。
 */

#ifndef YUNLINK_TRANSPORT_UDP_TRANSPORT_HPP
#define YUNLINK_TRANSPORT_UDP_TRANSPORT_HPP

#include <memory>
#include <string>

#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/protocol_codec.hpp"
#include "yunlink/core/runtime_config.hpp"

namespace yunlink {

class UdpTransport {
  public:
    UdpTransport();
    ~UdpTransport();

    ErrorCode start(const RuntimeConfig& config, EventBus* bus);
    void stop();

    bool is_running() const;
    bool running() const {
        return is_running();
    }

    int send_unicast(const ByteBuffer& bytes, const std::string& ip, uint16_t port);
    int send_broadcast(const ByteBuffer& bytes, uint16_t port);
    int send_multicast(const ByteBuffer& bytes, uint16_t port);

    int send_envelope_unicast(const SecureEnvelope& envelope, const std::string& ip, uint16_t port);
    int send_envelope_broadcast(const SecureEnvelope& envelope, uint16_t port);
    int send_envelope_multicast(const SecureEnvelope& envelope, uint16_t port);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    void recv_loop();
};

}  // namespace yunlink

#endif  // YUNLINK_TRANSPORT_UDP_TRANSPORT_HPP
