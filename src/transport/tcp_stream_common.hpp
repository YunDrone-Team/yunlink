/**
 * @file src/transport/tcp_stream_common.hpp
 * @brief TCP transport 共享 helper。
 */

#ifndef YUNLINK_TRANSPORT_TCP_STREAM_COMMON_HPP
#define YUNLINK_TRANSPORT_TCP_STREAM_COMMON_HPP

#include <array>
#include <chrono>
#include <limits>
#include <mutex>
#include <string>
#include <thread>

#include <asio.hpp>

#include "yunlink/core/envelope_stream_parser.hpp"
#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/runtime_config.hpp"

namespace yunlink {

inline std::string make_tcp_peer_id(const std::string& ip, uint16_t port) {
    return ip + ":" + std::to_string(port);
}

inline int tcp_bytes_to_int(size_t n) {
    return n > static_cast<size_t>(std::numeric_limits<int>::max()) ? -1 : static_cast<int>(n);
}

inline void
publish_tcp_link_event(EventBus* bus, TransportType transport, const PeerInfo& peer, bool is_up) {
    if (bus == nullptr) {
        return;
    }
    LinkEvent le;
    le.transport = transport;
    le.peer = peer;
    le.is_up = is_up;
    bus->publish_link(le);
}

inline void publish_tcp_envelope_event(EventBus* bus,
                                       TransportType transport,
                                       const PeerInfo& peer,
                                       const SecureEnvelope& envelope) {
    if (bus == nullptr) {
        return;
    }
    EnvelopeEvent ee;
    ee.transport = transport;
    ee.peer = peer;
    ee.envelope = envelope;
    bus->publish_envelope(ee);
}

inline void publish_tcp_packet_trace(EventBus* bus,
                                     const RuntimeConfig& config,
                                     PacketTraceDirection direction,
                                     PacketTraceStage stage,
                                     TransportType transport,
                                     const PeerInfo& peer,
                                     const SecureEnvelope* envelope,
                                     const uint8_t* raw_data,
                                     size_t raw_len,
                                     ErrorCode code = ErrorCode::kOk,
                                     const std::string& error_message = std::string()) {
    if (bus == nullptr || !config.packet_trace_enabled) {
        return;
    }
    bus->publish_packet_trace(make_packet_trace_record(direction,
                                                       stage,
                                                       transport,
                                                       peer,
                                                       envelope,
                                                       raw_data,
                                                       raw_len,
                                                       code,
                                                       error_message,
                                                       config.packet_trace_raw_preview_bytes,
                                                       config.packet_trace_payload_preview_bytes));
}

template <typename SocketLike>
int write_tcp_bytes(SocketLike& socket, std::mutex& send_mu, const ByteBuffer& bytes) {
    if (bytes.empty()) {
        return 0;
    }
    std::error_code ec;
    size_t sent = 0;
    {
        std::lock_guard<std::mutex> lock(send_mu);
        sent = asio::write(socket, asio::buffer(bytes), ec);
    }
    return ec ? -1 : tcp_bytes_to_int(sent);
}

template <typename SocketLike>
void run_tcp_read_loop(std::atomic<bool>& owner_running,
                       std::atomic<bool>& conn_running,
                       SocketLike& socket,
                       EnvelopeStreamParser& parser,
                       EventBus* bus,
                       const RuntimeConfig& config,
                       const PeerInfo& peer,
                       TransportType transport,
                       int io_poll_interval_ms) {
    std::array<uint8_t, 4096> buf{};

    while (owner_running.load() && conn_running.load()) {
        std::error_code ec;
        const size_t n = socket.read_some(asio::buffer(buf), ec);
        if (ec) {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(io_poll_interval_ms > 0 ? io_poll_interval_ms : 1));
                continue;
            }
            break;
        }

        publish_tcp_packet_trace(bus,
                                 config,
                                 PacketTraceDirection::kRx,
                                 PacketTraceStage::kRawReceived,
                                 transport,
                                 peer,
                                 nullptr,
                                 buf.data(),
                                 n);
        parser.feed(buf.data(), n);
        EnvelopeStreamParseEvent parse_event;
        while (parser.pop_next_event(&parse_event, config.packet_trace_raw_preview_bytes)) {
            if (parse_event.has_envelope && parse_event.result.ok()) {
                publish_tcp_packet_trace(bus,
                                         config,
                                         PacketTraceDirection::kRx,
                                         PacketTraceStage::kDecodeSucceeded,
                                         transport,
                                         peer,
                                         &parse_event.result.envelope,
                                         parse_event.raw.data(),
                                         parse_event.raw_len);
                publish_tcp_envelope_event(bus, transport, peer, parse_event.result.envelope);
            } else {
                publish_tcp_packet_trace(bus,
                                         config,
                                         PacketTraceDirection::kRx,
                                         PacketTraceStage::kDecodeFailed,
                                         transport,
                                         peer,
                                         nullptr,
                                         parse_event.raw.data(),
                                         parse_event.raw_len,
                                         parse_event.result.code,
                                         parse_event.error_message.empty()
                                             ? "tcp-decode-failed"
                                             : parse_event.error_message);
            }
        }
    }

    conn_running.store(false);
    publish_tcp_link_event(bus, transport, peer, false);
    std::error_code ec;
    socket.close(ec);
}

}  // namespace yunlink

#endif  // YUNLINK_TRANSPORT_TCP_STREAM_COMMON_HPP
