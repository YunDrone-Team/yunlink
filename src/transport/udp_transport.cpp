/**
 * @file src/transport/udp_transport.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/transport/udp_transport.hpp"

#include <cstring>

#include "sunraycom/core/message_ids.hpp"
#include "socket_common.hpp"

namespace sunraycom {

UdpTransport::UdpTransport() = default;

UdpTransport::~UdpTransport() {
    stop();
}

ErrorCode UdpTransport::start(const RuntimeConfig& config, EventBus* bus) {
    if (is_running_.load())
        return ErrorCode::kOk;
    if (bus == nullptr)
        return ErrorCode::kInvalidArgument;

    if (!socket_env_init())
        return ErrorCode::kSocketError;

    config_ = config;
    bus_ = bus;

    sock_ = static_cast<int>(socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP));
    if (sock_ == static_cast<int>(kInvalidSocket)) {
        return ErrorCode::kSocketError;
    }

    const int yes = 1;
    setsockopt(sock_, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char*>(&yes), sizeof(yes));
    setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&yes), sizeof(yes));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(config_.udp_bind_port);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sock_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        close_socket(sock_);
        sock_ = -1;
        return ErrorCode::kBindError;
    }

    is_running_.store(true);
    recv_thread_ = std::thread(&UdpTransport::recv_loop, this);
    return ErrorCode::kOk;
}

void UdpTransport::stop() {
    if (!is_running_.exchange(false))
        return;

    if (sock_ >= 0) {
        close_socket(sock_);
        sock_ = -1;
    }

    if (recv_thread_.joinable()) {
        recv_thread_.join();
    }

    std::lock_guard<std::mutex> lock(parser_mu_);
    parsers_.clear();
}

int UdpTransport::send_unicast(const ByteBuffer& bytes, const std::string& ip, uint16_t port) {
    if (sock_ < 0 || bytes.empty())
        return -1;
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &target.sin_addr) != 1)
        return -1;
    return static_cast<int>(sendto(sock_,
                                   reinterpret_cast<const char*>(bytes.data()),
                                   static_cast<int>(bytes.size()),
                                   0,
                                   reinterpret_cast<sockaddr*>(&target),
                                   sizeof(target)));
}

int UdpTransport::send_broadcast(const ByteBuffer& bytes, uint16_t port) {
    return send_unicast(bytes, "255.255.255.255", port);
}

int UdpTransport::send_multicast(const ByteBuffer& bytes, uint16_t port) {
    return send_unicast(bytes, config_.multicast_group, port);
}

int UdpTransport::send_frame_unicast(const Frame& frame, const std::string& ip, uint16_t port) {
    return send_unicast(codec_.encode(frame), ip, port);
}

int UdpTransport::send_frame_broadcast(const Frame& frame, uint16_t port) {
    return send_broadcast(codec_.encode(frame), port);
}

int UdpTransport::send_frame_multicast(const Frame& frame, uint16_t port) {
    return send_multicast(codec_.encode(frame), port);
}

void UdpTransport::recv_loop() {
    while (is_running_.load()) {
        fd_set rfds;
        FD_ZERO(&rfds);
        FD_SET(sock_, &rfds);
        timeval tv{};
        tv.tv_sec = 0;
        tv.tv_usec = 100 * 1000;

        const int ready = select(sock_ + 1, &rfds, nullptr, nullptr, &tv);
        if (ready <= 0)
            continue;

        sockaddr_in from{};
        socklen_t from_len = sizeof(from);
        uint8_t buf[4096];
#ifdef _WIN32
        const int n = recvfrom(sock_,
                               reinterpret_cast<char*>(buf),
                               sizeof(buf),
                               0,
                               reinterpret_cast<sockaddr*>(&from),
                               &from_len);
#else
        const int n = static_cast<int>(
            recvfrom(sock_, buf, sizeof(buf), 0, reinterpret_cast<sockaddr*>(&from), &from_len));
#endif
        if (n <= 0) {
            if (bus_) {
                ErrorEvent ee;
                ee.code = ErrorCode::kSocketError;
                ee.transport = TransportType::kUdpUnicast;
                ee.message = "udp recvfrom failed";
                bus_->publish_error(ee);
            }
            continue;
        }

        char ipstr[INET_ADDRSTRLEN] = {0};
        inet_ntop(AF_INET, &from.sin_addr, ipstr, sizeof(ipstr));
        const uint16_t port = ntohs(from.sin_port);
        const std::string ip = ipstr;
        const std::string peer_id = make_peer_id(ip, port);

        std::lock_guard<std::mutex> lock(parser_mu_);
        auto it = parsers_.find(peer_id);
        if (it == parsers_.end()) {
            it = parsers_.emplace(peer_id, FrameStreamParser(config_.max_buffer_bytes_per_peer))
                     .first;
        }

        it->second.feed(buf, static_cast<size_t>(n));

        Frame frame;
        DecodeResult dr;
        while (it->second.pop_next(&frame, &dr)) {
            FrameEvent ev;
            ev.transport = (frame.header.seq == static_cast<uint8_t>(MessageId::SearchMessageID))
                               ? TransportType::kUdpBroadcast
                               : TransportType::kUdpUnicast;
            ev.peer.id = peer_id;
            ev.peer.ip = ip;
            ev.peer.port = port;
            ev.frame = frame;
            if (bus_)
                bus_->publish_frame(ev);
        }
    }
}

}  // namespace sunraycom
