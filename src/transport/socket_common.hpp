/**
 * @file src/transport/socket_common.hpp
 * @brief yunlink source file.
 */

#ifndef YUNLINK_TRANSPORT_SOCKET_COMMON_HPP
#define YUNLINK_TRANSPORT_SOCKET_COMMON_HPP

#include <cerrno>
#include <cstring>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
using SocketHandle = SOCKET;
constexpr SocketHandle kInvalidSocket = INVALID_SOCKET;
inline int last_socket_error() {
    return WSAGetLastError();
}
inline void close_socket(SocketHandle s) {
    closesocket(s);
}
inline bool socket_env_init() {
    WSADATA dat;
    return WSAStartup(MAKEWORD(2, 2), &dat) == 0;
}
inline void socket_env_cleanup() {
    WSACleanup();
}
#else
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
using SocketHandle = int;
constexpr SocketHandle kInvalidSocket = -1;
inline int last_socket_error() {
    return errno;
}
inline void close_socket(SocketHandle s) {
    close(s);
}
inline bool socket_env_init() {
    return true;
}
inline void socket_env_cleanup() {}
#endif

inline std::string make_peer_id(const std::string& ip, uint16_t port) {
    return ip + ":" + std::to_string(port);
}

inline int set_nonblocking(SocketHandle s) {
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(s, FIONBIO, &mode);
#else
    const int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0)
        return -1;
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
#endif
}

inline int set_blocking(SocketHandle s) {
#ifdef _WIN32
    u_long mode = 0;
    return ioctlsocket(s, FIONBIO, &mode);
#else
    const int flags = fcntl(s, F_GETFL, 0);
    if (flags < 0)
        return -1;
    return fcntl(s, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

inline bool wait_writable(SocketHandle s, int timeout_ms) {
    fd_set wfds;
    FD_ZERO(&wfds);
    FD_SET(s, &wfds);
    timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    const int rc = select(static_cast<int>(s + 1), nullptr, &wfds, nullptr, &tv);
    return rc > 0 && FD_ISSET(s, &wfds);
}

inline bool check_connect_success(SocketHandle s) {
    int err = 0;
    socklen_t len = sizeof(err);
    if (getsockopt(s, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&err), &len) != 0) {
        return false;
    }
    return err == 0;
}

inline int send_all(SocketHandle s, const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0)
        return 0;
    size_t sent = 0;
    while (sent < len) {
#ifdef _WIN32
        const int rc =
            send(s, reinterpret_cast<const char*>(data + sent), static_cast<int>(len - sent), 0);
#else
        const int rc = static_cast<int>(send(s, data + sent, len - sent, 0));
#endif
        if (rc <= 0)
            return -1;
        sent += static_cast<size_t>(rc);
    }
    return static_cast<int>(sent);
}

#endif  // YUNLINK_TRANSPORT_SOCKET_COMMON_HPP
