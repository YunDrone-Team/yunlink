/**
 * @file src/transport/tcp_server.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/transport/tcp_server.hpp"

#include <vector>

#include "socket_common.hpp"

namespace sunraycom {

TcpServer::TcpServer() = default;

TcpServer::~TcpServer() {
    stop();
}

ErrorCode TcpServer::start(const RuntimeConfig& config, EventBus* bus) {
    if (is_running_.load())
        return ErrorCode::kOk;
    if (bus == nullptr)
        return ErrorCode::kInvalidArgument;
    if (!socket_env_init())
        return ErrorCode::kSocketError;

    config_ = config;
    bus_ = bus;

    listen_sock_ = static_cast<int>(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (listen_sock_ < 0)
        return ErrorCode::kSocketError;

    const int yes = 1;
    setsockopt(
        listen_sock_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.tcp_listen_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(listen_sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close_socket(listen_sock_);
        listen_sock_ = -1;
        return ErrorCode::kBindError;
    }
    if (listen(listen_sock_, 64) != 0) {
        close_socket(listen_sock_);
        listen_sock_ = -1;
        return ErrorCode::kListenError;
    }

    is_running_.store(true);
    accept_thread_ = std::thread(&TcpServer::accept_loop, this);
    return ErrorCode::kOk;
}

void TcpServer::stop() {
    if (!is_running_.exchange(false))
        return;

    if (listen_sock_ >= 0) {
        close_socket(listen_sock_);
        listen_sock_ = -1;
    }

    if (accept_thread_.joinable())
        accept_thread_.join();

    std::vector<std::shared_ptr<ClientConn>> clients;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& kv : clients_)
            clients.push_back(kv.second);
        clients_.clear();
    }

    for (auto& c : clients) {
        c->is_running.store(false);
        if (c->sock >= 0) {
            close_socket(c->sock);
            c->sock = -1;
        }
    }
    for (auto& c : clients) {
        if (c->rx_thread.joinable())
            c->rx_thread.join();
    }
}

int TcpServer::send(const std::string& peer_id, const ByteBuffer& bytes) {
    std::shared_ptr<ClientConn> conn;
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = clients_.find(peer_id);
        if (it == clients_.end())
            return -1;
        conn = it->second;
    }
    return send_all(conn->sock, bytes.data(), bytes.size());
}

int TcpServer::send_frame(const std::string& peer_id, const Frame& frame) {
    return send(peer_id, codec_.encode(frame));
}

void TcpServer::broadcast(const ByteBuffer& bytes) {
    std::vector<std::shared_ptr<ClientConn>> clients;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& kv : clients_)
            clients.push_back(kv.second);
    }
    for (const auto& c : clients) {
        send_all(c->sock, bytes.data(), bytes.size());
    }
}

void TcpServer::broadcast_frame(const Frame& frame) {
    broadcast(codec_.encode(frame));
}

void TcpServer::accept_loop() {
    while (is_running_.load()) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(listen_sock_, &rfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        const int ready = select(listen_sock_ + 1, &rfds, nullptr, nullptr, &tv);
        if (ready <= 0)
            continue;

        sockaddr_in cli{};
        socklen_t cli_len = sizeof(cli);
        const int client_sock =
            static_cast<int>(accept(listen_sock_, reinterpret_cast<sockaddr*>(&cli), &cli_len));
        if (client_sock < 0)
            continue;

        char ipstr[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &cli.sin_addr, ipstr, sizeof(ipstr));
        const uint16_t port = ntohs(cli.sin_port);

        auto conn = std::make_shared<ClientConn>();
        conn->sock = client_sock;
        conn->peer.ip = ipstr;
        conn->peer.port = port;
        conn->peer.id = make_peer_id(conn->peer.ip, conn->peer.port);
        conn->is_running.store(true);

        {
            std::lock_guard<std::mutex> lock(mu_);
            clients_[conn->peer.id] = conn;
        }

        if (bus_) {
            LinkEvent le;
            le.transport = TransportType::kTcpServer;
            le.peer = conn->peer;
            le.is_up = true;
            bus_->publish_link(le);
        }

        conn->rx_thread = std::thread(&TcpServer::client_loop, this, conn);
    }
}

void TcpServer::client_loop(std::shared_ptr<ClientConn> conn) {
    uint8_t buf[4096];
    while (is_running_.load() && conn->is_running.load()) {
#ifdef _WIN32
        const int n = recv(conn->sock, reinterpret_cast<char*>(buf), sizeof(buf), 0);
#else
        const int n = static_cast<int>(recv(conn->sock, buf, sizeof(buf), 0));
#endif
        if (n <= 0)
            break;

        conn->parser.feed(buf, static_cast<size_t>(n));
        Frame frame;
        DecodeResult dr;
        while (conn->parser.pop_next(&frame, &dr)) {
            FrameEvent fe;
            fe.transport = TransportType::kTcpServer;
            fe.peer = conn->peer;
            fe.frame = frame;
            if (bus_)
                bus_->publish_frame(fe);
        }
    }

    conn->is_running.store(false);
    if (bus_) {
        LinkEvent le;
        le.transport = TransportType::kTcpServer;
        le.peer = conn->peer;
        le.is_up = false;
        bus_->publish_link(le);
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = clients_.find(conn->peer.id);
        if (it != clients_.end() && it->second.get() == conn.get()) {
            clients_.erase(it);
        }
    }

    if (conn->sock >= 0) {
        close_socket(conn->sock);
        conn->sock = -1;
    }

    if (conn->rx_thread.joinable() && conn->rx_thread.get_id() == std::this_thread::get_id()) {
        conn->rx_thread.detach();
    }
}

}  // namespace sunraycom
