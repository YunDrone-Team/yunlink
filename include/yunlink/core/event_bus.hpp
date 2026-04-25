/**
 * @file include/yunlink/core/event_bus.hpp
 * @brief 事件总线定义。
 */

#ifndef YUNLINK_CORE_EVENT_BUS_HPP
#define YUNLINK_CORE_EVENT_BUS_HPP

#include <cstddef>
#include <functional>
#include <mutex>
#include <unordered_map>

#include "yunlink/core/types.hpp"

namespace yunlink {

struct EnvelopeEvent {
    TransportType transport = TransportType::kUdpUnicast;
    PeerInfo peer;
    SecureEnvelope envelope;
};

struct ErrorEvent {
    ErrorCode code = ErrorCode::kOk;
    TransportType transport = TransportType::kUdpUnicast;
    PeerInfo peer;
    std::string message;
};

struct LinkEvent {
    TransportType transport = TransportType::kTcpClient;
    PeerInfo peer;
    bool is_up = false;
};

class EventBus {
  public:
    using EnvelopeHandler = std::function<void(const EnvelopeEvent&)>;
    using ErrorHandler = std::function<void(const ErrorEvent&)>;
    using LinkHandler = std::function<void(const LinkEvent&)>;

    size_t subscribe_envelope(EnvelopeHandler cb);
    size_t subscribe_error(ErrorHandler cb);
    size_t subscribe_link(LinkHandler cb);
    void unsubscribe(size_t token);

    void publish_envelope(const EnvelopeEvent& ev) const;
    void publish_error(const ErrorEvent& ev) const;
    void publish_link(const LinkEvent& ev) const;

  private:
    mutable std::mutex mu_;
    size_t next_token_ = 1;
    std::unordered_map<size_t, EnvelopeHandler> envelope_handlers_;
    std::unordered_map<size_t, ErrorHandler> error_handlers_;
    std::unordered_map<size_t, LinkHandler> link_handlers_;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_EVENT_BUS_HPP
