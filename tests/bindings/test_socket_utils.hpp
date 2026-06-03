#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace yunlink::test_socket {

enum class SocketProtocol {
    kTcp,
    kUdp,
};

#ifdef _WIN32
using SocketHandle = SOCKET;
using SocketLength = int;
constexpr SocketHandle kInvalidSocketHandle = INVALID_SOCKET;

class SocketEnvGuard {
  public:
    SocketEnvGuard() : ok_(WSAStartup(MAKEWORD(2, 2), &data_) == 0) {}

    ~SocketEnvGuard() {
        if (ok_) {
            WSACleanup();
        }
    }

    bool ok() const {
        return ok_;
    }

  private:
    WSADATA data_{};
    bool ok_{false};
};

inline void close_socket(SocketHandle fd) {
    closesocket(fd);
}
#else
using SocketHandle = int;
using SocketLength = socklen_t;
constexpr SocketHandle kInvalidSocketHandle = -1;

class SocketEnvGuard {
  public:
    bool ok() const {
        return true;
    }
};

inline void close_socket(SocketHandle fd) {
    close(fd);
}
#endif

inline int socket_type(SocketProtocol protocol) {
    return protocol == SocketProtocol::kTcp ? SOCK_STREAM : SOCK_DGRAM;
}

inline int socket_ip_protocol(SocketProtocol protocol) {
    return protocol == SocketProtocol::kTcp ? IPPROTO_TCP : IPPROTO_UDP;
}

inline uint16_t find_free_port(SocketProtocol protocol) {
    SocketEnvGuard socket_env;
    if (!socket_env.ok()) {
        return 0;
    }

    const SocketHandle fd = ::socket(AF_INET, socket_type(protocol), socket_ip_protocol(protocol));
    if (fd == kInvalidSocketHandle) {
        return 0;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(0);
    if (::bind(fd, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) != 0) {
        close_socket(fd);
        return 0;
    }

    SocketLength len = sizeof(addr);
    if (::getsockname(fd, reinterpret_cast<sockaddr*>(&addr), &len) != 0) {
        close_socket(fd);
        return 0;
    }

    const uint16_t port = ntohs(addr.sin_port);
    close_socket(fd);
    return port;
}

template <std::size_t N>
inline uint16_t find_unique_free_port(SocketProtocol protocol,
                                      const std::array<uint16_t, N>& used_ports,
                                      int attempts = 64) {
    for (int i = 0; i < attempts; ++i) {
        const uint16_t port = find_free_port(protocol);
        if (port == 0) {
            continue;
        }
        if (std::find(used_ports.begin(), used_ports.end(), port) == used_ports.end()) {
            return port;
        }
    }
    return 0;
}

}  // namespace yunlink::test_socket
