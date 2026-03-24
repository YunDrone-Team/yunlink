/**
 * @file src/transport/tcp_client_pool.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/transport/tcp_client_pool.hpp"

#include <vector>

#include "socket_common.hpp"

namespace sunraycom {

TcpClientPool::TcpClientPool() = default;

TcpClientPool::~TcpClientPool() {
    stop();
}

ErrorCode TcpClientPool::start(const RuntimeConfig& config, EventBus* bus) {
    if (is_running_.load())
        return ErrorCode::kOk;
    if (bus == nullptr)
        return ErrorCode::kInvalidArgument;
    if (!socket_env_init())
        return ErrorCode::kSocketError;
    config_ = config;
    bus_ = bus;
    is_running_.store(true);
    return ErrorCode::kOk;
}

void TcpClientPool::stop() {
    if (!is_running_.exchange(false))
        return;

    std::vector<std::shared_ptr<PeerConn>> peers;
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& kv : peers_)
            peers.push_back(kv.second);
        peers_.clear();
    }

    for (auto& conn : peers) {
        conn->is_running.store(false);
        if (conn->sock >= 0) {
            close_socket(conn->sock);
            conn->sock = -1;
        }
    }
    for (auto& conn : peers) {
        if (conn->rx_thread.joinable())
            conn->rx_thread.join();
    }
}

ErrorCode
TcpClientPool::connect_peer(const std::string& ip, uint16_t port, std::string* out_peer_id) {
    if (!is_running_.load())
        return ErrorCode::kRuntimeStopped;

    const std::string peer_id = make_peer_id(ip, port);
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (peers_.find(peer_id) != peers_.end()) {
            if (out_peer_id)
                *out_peer_id = peer_id;
            return ErrorCode::kOk;
        }
    }

    SocketHandle sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == kInvalidSocket)
        return ErrorCode::kSocketError;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        close_socket(sock);
        return ErrorCode::kInvalidArgument;
    }

    set_nonblocking(sock);
    int rc = ::connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    bool ok = false;
    if (rc == 0) {
        ok = true;
    } else {
        if (wait_writable(sock, config_.connect_timeout_ms) && check_connect_success(sock)) {
            ok = true;
        }
    }

    if (!ok) {
        close_socket(sock);
        if (bus_) {
            ErrorEvent ee;
            ee.code = ErrorCode::kConnectError;
            ee.transport = TransportType::kTcpClient;
            ee.peer.id = peer_id;
            ee.peer.ip = ip;
            ee.peer.port = port;
            ee.message = "tcp connect failed";
            bus_->publish_error(ee);
        }
        return ErrorCode::kConnectError;
    }

    set_blocking(sock);

    auto conn = std::make_shared<PeerConn>();
    conn->sock = static_cast<int>(sock);
    conn->peer.id = peer_id;
    conn->peer.ip = ip;
    conn->peer.port = port;
    conn->is_running.store(true);

    {
        std::lock_guard<std::mutex> lock(mu_);
        peers_[peer_id] = conn;
    }

    if (bus_) {
        LinkEvent le;
        le.transport = TransportType::kTcpClient;
        le.peer = conn->peer;
        le.is_up = true;
        bus_->publish_link(le);
    }

    conn->rx_thread = std::thread(&TcpClientPool::peer_loop, this, conn);

    if (out_peer_id)
        *out_peer_id = peer_id;
    return ErrorCode::kOk;
}

void TcpClientPool::close_peer(const std::string& peer_id) {
    std::shared_ptr<PeerConn> conn;
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = peers_.find(peer_id);
        if (it == peers_.end())
            return;
        conn = it->second;
        peers_.erase(it);
    }

    conn->is_running.store(false);
    if (conn->sock >= 0) {
        close_socket(conn->sock);
        conn->sock = -1;
    }
    if (conn->rx_thread.joinable())
        conn->rx_thread.join();
}

int TcpClientPool::send(const std::string& peer_id, const ByteBuffer& bytes) {
    std::shared_ptr<PeerConn> conn;
    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = peers_.find(peer_id);
        if (it == peers_.end())
            return -1;
        conn = it->second;
    }
    return send_all(conn->sock, bytes.data(), bytes.size());
}

int TcpClientPool::send_frame(const std::string& peer_id, const Frame& frame) {
    return send(peer_id, codec_.encode(frame));
}

void TcpClientPool::peer_loop(std::shared_ptr<PeerConn> conn) {
    uint8_t buf[4096];
    while (is_running_.load() && conn->is_running.load()) {
#ifdef _WIN32
        const int n = recv(conn->sock, reinterpret_cast<char*>(buf), sizeof(buf), 0);
#else
        const int n = static_cast<int>(recv(conn->sock, buf, sizeof(buf), 0));
#endif
        if (n <= 0) {
            break;
        }

        conn->parser.feed(buf, static_cast<size_t>(n));
        Frame frame;
        DecodeResult dr;
        while (conn->parser.pop_next(&frame, &dr)) {
            FrameEvent fe;
            fe.transport = TransportType::kTcpClient;
            fe.peer = conn->peer;
            fe.frame = frame;
            if (bus_)
                bus_->publish_frame(fe);
        }
    }

    conn->is_running.store(false);
    if (bus_) {
        LinkEvent le;
        le.transport = TransportType::kTcpClient;
        le.peer = conn->peer;
        le.is_up = false;
        bus_->publish_link(le);
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = peers_.find(conn->peer.id);
        if (it != peers_.end() && it->second.get() == conn.get()) {
            peers_.erase(it);
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
