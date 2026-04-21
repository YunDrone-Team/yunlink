/**
 * @file src/transport/tcp_server.cpp
 * @brief sunray_communication_lib source file.
 */

#include "sunraycom/transport/tcp_server.hpp"

#include <chrono>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

#include <asio.hpp>

#include "tcp_stream_common.hpp"

namespace sunraycom {

struct TcpServer::ClientConn {
    PeerInfo peer;
    std::atomic<bool> is_running{false};
    std::thread rx_thread;
    EnvelopeStreamParser parser;
    std::shared_ptr<asio::ip::tcp::socket> socket;
    std::mutex send_mu;

    explicit ClientConn(size_t max_buffer_bytes) : parser(max_buffer_bytes) {}
};

struct TcpServer::Impl {
    RuntimeConfig config;
    EventBus* bus = nullptr;
    ProtocolCodec codec;

    std::atomic<bool> is_running{false};
    asio::io_context accept_io;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
    std::thread accept_thread;

    std::mutex mu;
    std::unordered_map<std::string, std::shared_ptr<ClientConn>> clients;
};

bool is_socket_closed(const std::error_code& ec) {
    return ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor;
}

TcpServer::TcpServer() : impl_(std::make_unique<Impl>()) {}

TcpServer::~TcpServer() {
    stop();
}

ErrorCode TcpServer::start(const RuntimeConfig& config, EventBus* bus) {
    if (impl_->is_running.load()) {
        return ErrorCode::kOk;
    }
    if (bus == nullptr) {
        return ErrorCode::kInvalidArgument;
    }

    impl_->config = config;
    impl_->bus = bus;

    impl_->acceptor = std::make_unique<asio::ip::tcp::acceptor>(impl_->accept_io);

    std::error_code ec;
    impl_->acceptor->open(asio::ip::tcp::v4(), ec);
    if (ec) {
        impl_->acceptor.reset();
        return ErrorCode::kSocketError;
    }

    impl_->acceptor->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        impl_->acceptor.reset();
        return ErrorCode::kSocketError;
    }

    impl_->acceptor->bind(
        asio::ip::tcp::endpoint(asio::ip::tcp::v4(), impl_->config.tcp_listen_port), ec);
    if (ec) {
        impl_->acceptor.reset();
        return ErrorCode::kBindError;
    }

    impl_->acceptor->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        impl_->acceptor.reset();
        return ErrorCode::kListenError;
    }
    impl_->acceptor->non_blocking(true, ec);
    if (ec) {
        impl_->acceptor.reset();
        return ErrorCode::kSocketError;
    }

    impl_->is_running.store(true);
    impl_->accept_thread = std::thread(&TcpServer::accept_loop, this);
    return ErrorCode::kOk;
}

void TcpServer::stop() {
    if (!impl_->is_running.exchange(false)) {
        return;
    }

    if (impl_->acceptor) {
        std::error_code ec;
        impl_->acceptor->cancel(ec);
        impl_->acceptor->close(ec);
    }

    if (impl_->accept_thread.joinable()) {
        impl_->accept_thread.join();
    }

    std::vector<std::shared_ptr<ClientConn>> clients;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        for (auto& kv : impl_->clients) {
            clients.push_back(kv.second);
        }
        impl_->clients.clear();
    }

    for (const auto& c : clients) {
        c->is_running.store(false);
        if (c->socket) {
            std::error_code ec;
            c->socket->cancel(ec);
            c->socket->close(ec);
        }
    }

    for (const auto& c : clients) {
        if (c->rx_thread.joinable()) {
            c->rx_thread.join();
        }
    }

    impl_->acceptor.reset();
}

int TcpServer::send(const std::string& peer_id, const ByteBuffer& bytes) {
    std::shared_ptr<ClientConn> conn;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        auto it = impl_->clients.find(peer_id);
        if (it == impl_->clients.end()) {
            return -1;
        }
        conn = it->second;
    }

    if (!conn->socket) {
        return -1;
    }
    return write_tcp_bytes(*conn->socket, conn->send_mu, bytes);
}

int TcpServer::send_envelope(const std::string& peer_id, const SecureEnvelope& envelope) {
    return send(peer_id, impl_->codec.encode(envelope));
}

void TcpServer::broadcast(const ByteBuffer& bytes) {
    std::vector<std::shared_ptr<ClientConn>> clients;
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        for (auto& kv : impl_->clients) {
            clients.push_back(kv.second);
        }
    }

    for (const auto& c : clients) {
        if (!c->socket) {
            continue;
        }
        std::error_code ec;
        std::lock_guard<std::mutex> lock(c->send_mu);
        asio::write(*c->socket, asio::buffer(bytes), ec);
    }
}

void TcpServer::broadcast_envelope(const SecureEnvelope& envelope) {
    broadcast(impl_->codec.encode(envelope));
}

void TcpServer::accept_loop() {
    while (impl_->is_running.load()) {
        auto socket = std::make_shared<asio::ip::tcp::socket>(impl_->accept_io);
        std::error_code ec;
        impl_->acceptor->accept(*socket, ec);
        if (ec) {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    impl_->config.io_poll_interval_ms > 0 ? impl_->config.io_poll_interval_ms : 1));
                continue;
            }
            if (!impl_->is_running.load() || is_socket_closed(ec)) {
                break;
            }
            continue;
        }

        const auto endpoint = socket->remote_endpoint(ec);
        if (ec) {
            socket->close(ec);
            continue;
        }

        const std::string ip = endpoint.address().to_string(ec);
        if (ec) {
            socket->close(ec);
            continue;
        }

        auto conn = std::make_shared<ClientConn>(impl_->config.max_buffer_bytes_per_peer);
        conn->socket = std::move(socket);
        conn->socket->non_blocking(true, ec);
        conn->peer.ip = ip;
        conn->peer.port = endpoint.port();
        conn->peer.id = make_tcp_peer_id(conn->peer.ip, conn->peer.port);
        conn->is_running.store(true);

        {
            std::lock_guard<std::mutex> lock(impl_->mu);
            impl_->clients[conn->peer.id] = conn;
        }

        publish_tcp_link_event(impl_->bus, TransportType::kTcpServer, conn->peer, true);

        conn->rx_thread = std::thread(&TcpServer::client_loop, this, conn);
    }
}

void TcpServer::client_loop(const std::shared_ptr<ClientConn>& conn) {
    run_tcp_read_loop(impl_->is_running,
                      conn->is_running,
                      *conn->socket,
                      conn->parser,
                      impl_->bus,
                      conn->peer,
                      TransportType::kTcpServer,
                      impl_->config.io_poll_interval_ms);
}

}  // namespace sunraycom
