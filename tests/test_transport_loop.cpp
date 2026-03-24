/**
 * @file tests/test_transport_loop.cpp
 * @brief SunrayComLib source file.
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "sunraycom/core/protocol_codec.hpp"
#include "sunraycom/runtime/runtime.hpp"

int main() {
    sunraycom::Runtime runtime;
    sunraycom::RuntimeConfig cfg;
    cfg.udp_bind_port = 12096;
    cfg.udp_target_port = 12096;
    cfg.tcp_listen_port = 12196;

    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::atomic<bool> got{false};
    auto tok = runtime.event_bus().subscribe_frame([&got](const sunraycom::FrameEvent& ev) {
        if (ev.transport == sunraycom::TransportType::kUdpUnicast && ev.frame.header.seq == 201) {
            got.store(true);
        }
    });

    sunraycom::Frame f;
    f.header.seq = 201;
    f.header.robot_id = 1;
    f.payload = {1, 9, 9};
    sunraycom::ProtocolCodec codec;
    const auto bytes = codec.encode(f, true);
    runtime.udp().send_unicast(bytes, "127.0.0.1", cfg.udp_bind_port);

    for (int i = 0; i < 40 && !got.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    runtime.event_bus().unsubscribe(tok);
    runtime.stop();

    if (!got.load()) {
        std::cerr << "did not receive looped udp frame\n";
        return 2;
    }

    return 0;
}
