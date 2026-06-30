/**
 * @file src/transport/udp_transport.cpp
 * @brief yunlink source file.
 */

#include "yunlink/transport/udp_transport.hpp"

#include <array>
#include <chrono>
#include <limits>
#include <thread>
#include <unordered_map>
#include <vector>

#include <asio.hpp>

#include "yunlink/core/envelope_stream_parser.hpp"
#include "udp_packet_trace.hpp"

namespace yunlink {

namespace {

std::string make_peer_id(const std::string& ip, uint16_t port) {
    return ip + ":" + std::to_string(port);
}

int to_int_bytes(size_t n) {
    if (n > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return -1;
    }
    return static_cast<int>(n);
}

bool is_socket_closed(const std::error_code& ec) {
    return ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor;
}

PeerInfo make_udp_peer(const std::string& ip, uint16_t port) {
    PeerInfo peer;
    peer.id = make_peer_id(ip, port);
    peer.ip = ip;
    peer.port = port;
    return peer;
}

}  // namespace

struct UdpTransport::Impl {
    RuntimeConfig config;
    EventBus* bus = nullptr;
    ProtocolCodec codec;

    std::atomic<bool> is_running{false};
    asio::io_context io;
    std::unique_ptr<asio::ip::udp::socket> socket;
    std::thread recv_thread;

    std::mutex parser_mu;
    std::unordered_map<std::string, EnvelopeStreamParser> parsers;
};

UdpTransport::UdpTransport() : impl_(std::make_unique<Impl>()) {}

UdpTransport::~UdpTransport() {
    stop();
}

bool UdpTransport::is_running() const {
    return impl_->is_running.load();
}

ErrorCode UdpTransport::start(const RuntimeConfig& config, EventBus* bus) {
    if (impl_->is_running.load()) {
        return ErrorCode::kOk;
    }
    if (bus == nullptr) {
        return ErrorCode::kInvalidArgument;
    }

    impl_->config = config;
    impl_->bus = bus;

    impl_->socket = std::make_unique<asio::ip::udp::socket>(impl_->io);

    std::error_code ec;
    impl_->socket->open(asio::ip::udp::v4(), ec);
    if (ec) {
        impl_->socket.reset();
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::broadcast(true), ec);
    if (ec) {
        impl_->socket.reset();
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        impl_->socket.reset();
        return ErrorCode::kSocketError;
    }

    impl_->socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), impl_->config.udp_bind_port),
                        ec);
    if (ec) {
        impl_->socket.reset();
        return ErrorCode::kBindError;
    }
    impl_->socket->non_blocking(true, ec);
    if (ec) {
        impl_->socket.reset();
        return ErrorCode::kSocketError;
    }

    impl_->is_running.store(true);
    impl_->recv_thread = std::thread(&UdpTransport::recv_loop, this);
    return ErrorCode::kOk;
}

void UdpTransport::stop() {
    if (!impl_->is_running.exchange(false)) {
        return;
    }

    if (impl_->socket) {
        std::error_code ec;
        impl_->socket->cancel(ec);
        impl_->socket->close(ec);
    }

    if (impl_->recv_thread.joinable()) {
        impl_->recv_thread.join();
    }

    std::lock_guard<std::mutex> lock(impl_->parser_mu);
    impl_->parsers.clear();
    impl_->socket.reset();
}

int UdpTransport::send_unicast(const ByteBuffer& bytes, const std::string& ip, uint16_t port) {
    if (!impl_->socket || bytes.empty()) {
        return -1;
    }

    std::error_code ec;
    const auto address = asio::ip::make_address(ip, ec);
    if (ec) {
        return -1;
    }

    const asio::ip::udp::endpoint endpoint(address, port);
    const size_t sent = impl_->socket->send_to(asio::buffer(bytes), endpoint, 0, ec);
    if (ec) {
        return -1;
    }

    return to_int_bytes(sent);
}

int UdpTransport::send_broadcast(const ByteBuffer& bytes, uint16_t port) {
    return send_unicast(bytes, "255.255.255.255", port);
}

int UdpTransport::send_multicast(const ByteBuffer& bytes, uint16_t port) {
    return send_unicast(bytes, impl_->config.multicast_group, port);
}

int UdpTransport::send_envelope_unicast(const SecureEnvelope& envelope,
                                        const std::string& ip,
                                        uint16_t port) {
    return send_unicast(impl_->codec.encode(envelope), ip, port);
}

int UdpTransport::send_envelope_broadcast(const SecureEnvelope& envelope, uint16_t port) {
    return send_broadcast(impl_->codec.encode(envelope), port);
}

int UdpTransport::send_envelope_multicast(const SecureEnvelope& envelope, uint16_t port) {
    return send_multicast(impl_->codec.encode(envelope), port);
}

void UdpTransport::recv_loop() {
    std::array<uint8_t, 4096> buf{};

    while (impl_->is_running.load()) {
        std::error_code ec;
        asio::ip::udp::endpoint from;
        const size_t n = impl_->socket->receive_from(asio::buffer(buf), from, 0, ec);

        if (ec) {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    impl_->config.io_poll_interval_ms > 0 ? impl_->config.io_poll_interval_ms : 1));
                continue;
            }
            if (!impl_->is_running.load() || is_socket_closed(ec)) {
                break;
            }

            if (impl_->bus) {
                ErrorEvent ee;
                ee.code = ErrorCode::kSocketError;
                ee.transport = TransportType::kUdpUnicast;
                ee.message = std::string("udp receive failed: ") + ec.message();
                impl_->bus->publish_error(ee);
            }
            continue;
        }

        const std::string ip = from.address().to_string(ec);
        if (ec) {
            continue;
        }
        const uint16_t port = from.port();
        const PeerInfo peer = make_udp_peer(ip, port);

        publish_udp_packet_trace(impl_->bus,
                                 impl_->config,
                                 PacketTraceStage::kRawReceived,
                                 peer,
                                 nullptr,
                                 buf.data(),
                                 n);

        std::vector<SecureEnvelope> envelopes;
        std::vector<EnvelopeStreamParseEvent> parse_events;
        {
            std::lock_guard<std::mutex> lock(impl_->parser_mu);
            auto it = impl_->parsers.find(peer.id);
            if (it == impl_->parsers.end()) {
                it = impl_->parsers
                         .emplace(peer.id,
                                  EnvelopeStreamParser(impl_->config.max_buffer_bytes_per_peer,
                                                       impl_->config.max_buffer_bytes_per_peer))
                         .first;
            }

            it->second.feed(buf.data(), n);
            EnvelopeStreamParseEvent parse_event;
            while (it->second.pop_next_event(&parse_event,
                                             impl_->config.packet_trace_raw_preview_bytes)) {
                parse_events.push_back(parse_event);
                if (parse_event.has_envelope && parse_event.result.ok()) {
                    envelopes.push_back(parse_event.result.envelope);
                }
            }
        }

        publish_udp_parse_events(impl_->bus, impl_->config, peer, parse_events);
        publish_udp_envelopes(impl_->bus, peer, envelopes);
    }
}

}  // namespace yunlink
