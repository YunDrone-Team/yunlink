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
        const std::string peer_id = make_peer_id(ip, port);

        std::vector<SecureEnvelope> envelopes;
        {
            std::lock_guard<std::mutex> lock(impl_->parser_mu);
            auto it = impl_->parsers.find(peer_id);
            if (it == impl_->parsers.end()) {
                it = impl_->parsers
                         .emplace(peer_id,
                                  EnvelopeStreamParser(impl_->config.max_buffer_bytes_per_peer))
                         .first;
            }

            it->second.feed(buf.data(), n);
            SecureEnvelope envelope;
            DecodeResult dr;
            while (it->second.pop_next(&envelope, &dr)) {
                envelopes.push_back(envelope);
            }
        }

        for (const auto& envelope : envelopes) {
            EnvelopeEvent ev;
            ev.transport = (envelope.target.scope == TargetScope::kBroadcast)
                               ? TransportType::kUdpBroadcast
                               : TransportType::kUdpUnicast;
            ev.peer.id = peer_id;
            ev.peer.ip = ip;
            ev.peer.port = port;
            ev.envelope = envelope;
            if (impl_->bus) {
                impl_->bus->publish_envelope(ev);
            }
        }
    }
}

}  // namespace yunlink
