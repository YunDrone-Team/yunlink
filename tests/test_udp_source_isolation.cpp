/**
 * @file tests/test_udp_source_isolation.cpp
 * @brief SunrayComLib source file.
 */

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#include "sunraycom/runtime/runtime.hpp"

namespace {

int send_part(int sock,
              const std::string& ip,
              uint16_t port,
              const std::vector<uint8_t>& data,
              size_t begin,
              size_t end) {
    sockaddr_in target{};
    target.sin_family = AF_INET;
    target.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &target.sin_addr);
    return static_cast<int>(sendto(sock,
                                   data.data() + begin,
                                   end - begin,
                                   0,
                                   reinterpret_cast<sockaddr*>(&target),
                                   sizeof(target)));
}

}  // namespace

int main() {
    sunraycom::Runtime runtime;
    sunraycom::RuntimeConfig cfg;
    cfg.udp_bind_port = 12100;
    cfg.udp_target_port = 12100;
    cfg.tcp_listen_port = 12200;
    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::atomic<int> got{0};
    auto tok = runtime.event_bus().subscribe_frame([&got](const sunraycom::FrameEvent& ev) {
        if (ev.transport == sunraycom::TransportType::kUdpUnicast &&
            (ev.frame.header.seq == 201 || ev.frame.header.seq == 202)) {
            ++got;
        }
    });

    sunraycom::ProtocolCodec codec;
    sunraycom::Frame f1;
    f1.header.seq = 201;
    f1.header.robot_id = 1;
    f1.payload = {1, 2, 3, 4, 5, 6};
    auto b1 = codec.encode(f1, true);

    sunraycom::Frame f2;
    f2.header.seq = 202;
    f2.header.robot_id = 2;
    f2.payload = {9, 8, 7, 6, 5, 4};
    auto b2 = codec.encode(f2, true);

    int s1 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    int s2 = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s1 < 0 || s2 < 0) {
        std::cerr << "socket create failed\n";
        return 2;
    }

    size_t h1 = b1.size() / 2;
    size_t h2 = b2.size() / 2;

    send_part(s1, "127.0.0.1", cfg.udp_bind_port, b1, 0, h1);
    send_part(s2, "127.0.0.1", cfg.udp_bind_port, b2, 0, h2);
    send_part(s1, "127.0.0.1", cfg.udp_bind_port, b1, h1, b1.size());
    send_part(s2, "127.0.0.1", cfg.udp_bind_port, b2, h2, b2.size());

    for (int i = 0; i < 100 && got.load() < 2; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    close(s1);
    close(s2);
    runtime.event_bus().unsubscribe(tok);
    runtime.stop();

    if (got.load() < 2) {
        std::cerr << "source isolation decode failed, got=" << got.load() << "\n";
        return 3;
    }

    return 0;
}
