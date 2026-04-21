/**
 * @file src/transport/tcp_client_pool.cpp
 * @brief sunray_communication_lib source file.
 */

#include "sunraycom/transport/tcp_client_pool.hpp"

#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <asio.hpp>

#include "tcp_stream_common.hpp"

namespace sunraycom {

struct TcpClientPool::PeerConn {
    PeerInfo peer;
    std::atomic<bool> is_running{false};
    std::thread rx_thread;
    EnvelopeStreamParser parser;

    asio::io_context io;
    asio::ip::tcp::socket socket;
    std::mutex send_mu;

    explicit PeerConn(size_t max_buffer_bytes) : parser(max_buffer_bytes), socket(io) {}
};

struct TcpClientPool::Impl {
    RuntimeConfig config;
    EventBus* bus = nullptr;
    ProtocolCodec codec;

    std::atomic<bool> is_running{false};
    std::mutex mu;
    std::unordered_map<std::string, std::shared_ptr<PeerConn>> peers;
};

std::error_code connect_with_timeout(asio::ip::tcp::socket& socket,
                                     const asio::ip::tcp::endpoint& endpoint,
                                     int timeout_ms) {
    auto& io = static_cast<asio::io_context&>(socket.get_executor().context());

    std::error_code connect_ec = asio::error::would_block;
    bool timed_out = false;

    asio::steady_timer timer(io);
    socket.async_connect(endpoint, [&](const std::error_code& ec) {
        connect_ec = ec;
        std::error_code cancel_ec;
        timer.cancel(cancel_ec);
    });

    timer.expires_after(std::chrono::milliseconds(timeout_ms > 0 ? timeout_ms : 1));
    timer.async_wait([&](const std::error_code& ec) {
        if (!ec) {
            timed_out = true;
            std::error_code cancel_ec;
            socket.cancel(cancel_ec);
            socket.close(cancel_ec);
        }
    });

    io.run();
    io.restart();

    if (timed_out) {
        return asio::error::timed_out;
    }
    return connect_ec;
}

TcpClientPool::TcpClientPool() : impl_(std::make_unique<Impl>()) {}

TcpClientPool::~TcpClientPool() {
    stop();
}

ErrorCode TcpClientPool::start(const RuntimeConfig& config, EventBus* bus) {
    if (impl_->is_running.load()) {
        return ErrorCode::kOk;
    }
    if (bus == nullptr) {
        return ErrorCode::kInvalidArgument;
    }

    impl_->config = config;
    impl_->bus = bus;
    impl_->is_running.store(true);
    return ErrorCode::kOk;
}

void TcpClientPool::stop() {
    if (!impl_->is_running.exchange(false)) {
        return;
    }

    std::vector<std::shared_ptr<PeerConn>> peers;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        for (auto& kv : impl_->peers) {
            peers.push_back(kv.second);
        }
        impl_->peers.clear();
    }

    for (const auto& conn : peers) {
        conn->is_running.store(false);
        std::error_code ec;
        conn->socket.cancel(ec);
        conn->socket.close(ec);
    }

    for (const auto& conn : peers) {
        if (conn->rx_thread.joinable()) {
            conn->rx_thread.join();
        }
    }
}

ErrorCode
TcpClientPool::connect_peer(const std::string& ip, uint16_t port, std::string* out_peer_id) {
    if (!impl_->is_running.load()) {
        return ErrorCode::kRuntimeStopped;
    }

    const std::string peer_id = make_tcp_peer_id(ip, port);
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it = impl_->peers.find(peer_id);
        if (it != impl_->peers.end()) {
            if (it->second->is_running.load()) {
                if (out_peer_id) {
                    *out_peer_id = peer_id;
                }
                return ErrorCode::kOk;
            }
            impl_->peers.erase(it);
        }
    }

    std::error_code ec;
    const auto address = asio::ip::make_address(ip, ec);
    if (ec) {
        return ErrorCode::kInvalidArgument;
    }

    auto conn = std::make_shared<PeerConn>(impl_->config.max_buffer_bytes_per_peer);
    conn->peer.id = peer_id;
    conn->peer.ip = ip;
    conn->peer.port = port;

    conn->socket.open(asio::ip::tcp::v4(), ec);
    if (ec) {
        return ErrorCode::kSocketError;
    }

    const std::error_code connect_ec = connect_with_timeout(
        conn->socket, asio::ip::tcp::endpoint(address, port), impl_->config.connect_timeout_ms);

    if (connect_ec) {
        std::error_code close_ec;
        conn->socket.close(close_ec);

        if (impl_->bus) {
            ErrorEvent ee;
            ee.code = (connect_ec == asio::error::timed_out) ? ErrorCode::kTimeout
                                                             : ErrorCode::kConnectError;
            ee.transport = TransportType::kTcpClient;
            ee.peer.id = peer_id;
            ee.peer.ip = ip;
            ee.peer.port = port;
            ee.message = std::string("tcp connect failed: ") + connect_ec.message();
            impl_->bus->publish_error(ee);
        }

        return (connect_ec == asio::error::timed_out) ? ErrorCode::kTimeout
                                                      : ErrorCode::kConnectError;
    }

    conn->is_running.store(true);
    conn->socket.non_blocking(true, ec);

    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        impl_->peers[peer_id] = conn;
    }

    publish_tcp_link_event(impl_->bus, TransportType::kTcpClient, conn->peer, true);

    conn->rx_thread = std::thread(&TcpClientPool::peer_loop, this, conn);

    if (out_peer_id) {
        *out_peer_id = peer_id;
    }
    return ErrorCode::kOk;
}

void TcpClientPool::close_peer(const std::string& peer_id) {
    std::shared_ptr<PeerConn> conn;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it = impl_->peers.find(peer_id);
        if (it == impl_->peers.end()) {
            return;
        }
        conn = it->second;
        impl_->peers.erase(it);
    }

    conn->is_running.store(false);
    std::error_code ec;
    conn->socket.cancel(ec);
    conn->socket.close(ec);

    if (conn->rx_thread.joinable() && conn->rx_thread.get_id() != std::this_thread::get_id()) {
        conn->rx_thread.join();
    }
}

int TcpClientPool::send(const std::string& peer_id, const ByteBuffer& bytes) {
    std::shared_ptr<PeerConn> conn;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it = impl_->peers.find(peer_id);
        if (it == impl_->peers.end()) {
            return -1;
        }
        conn = it->second;
    }

    return write_tcp_bytes(conn->socket, conn->send_mu, bytes);
}

int TcpClientPool::send_envelope(const std::string& peer_id, const SecureEnvelope& envelope) {
    return send(peer_id, impl_->codec.encode(envelope));
}

void TcpClientPool::peer_loop(const std::shared_ptr<PeerConn>& conn) {
    run_tcp_read_loop(impl_->is_running,
                      conn->is_running,
                      conn->socket,
                      conn->parser,
                      impl_->bus,
                      conn->peer,
                      TransportType::kTcpClient,
                      impl_->config.io_poll_interval_ms);
}

}  // namespace sunraycom
