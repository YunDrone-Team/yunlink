/**
 * @file tests/test_udp_source_isolation.cpp
 * @brief sunray_communication_lib source file.
 */

#include <asio.hpp>

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/runtime/runtime.hpp"

namespace {

int send_part(asio::ip::udp::socket& sock,
              const std::string& ip,
              uint16_t port,
              const std::vector<uint8_t>& data,
              size_t begin,
              size_t end) {
    std::error_code ec;
    const auto addr = asio::ip::make_address(ip, ec);
    if (ec) {
        return -1;
    }

    const asio::ip::udp::endpoint target(addr, port);
    const size_t sent = sock.send_to(asio::buffer(data.data() + begin, end - begin), target, 0, ec);
    return ec ? -1 : static_cast<int>(sent);
}

}  // namespace

int main() {
    sunraycom::Runtime runtime;
    sunraycom::RuntimeConfig cfg;
    cfg.udp_bind_port = 12100;
    cfg.udp_target_port = 12100;
    cfg.tcp_listen_port = 12200;
    cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    cfg.self_identity.agent_id = 77;
    cfg.self_identity.role = sunraycom::EndpointRole::kObserver;
    if (runtime.start(cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::atomic<int> got{0};
    auto tok = runtime.event_bus().subscribe_envelope([&got](const sunraycom::EnvelopeEvent& ev) {
        if (ev.transport == sunraycom::TransportType::kUdpUnicast &&
            ev.envelope.message_family == sunraycom::MessageFamily::kStateEvent) {
            ++got;
        }
    });

    sunraycom::ProtocolCodec codec;
    sunraycom::VehicleEvent e1{};
    e1.kind = sunraycom::VehicleEventKind::kFault;
    e1.severity = 3;
    e1.detail = "camera";
    auto b1 = codec.encode(
        sunraycom::make_typed_envelope(
            {sunraycom::AgentType::kUav, 1, sunraycom::EndpointRole::kVehicle},
            sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kGroundStation, 77),
            0,
            201,
            sunraycom::QosClass::kBestEffort,
            e1),
        true);

    sunraycom::VehicleEvent e2{};
    e2.kind = sunraycom::VehicleEventKind::kFormationUpdate;
    e2.severity = 1;
    e2.detail = "swarm";
    auto b2 = codec.encode(
        sunraycom::make_typed_envelope(
            {sunraycom::AgentType::kUav, 2, sunraycom::EndpointRole::kVehicle},
            sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kGroundStation, 77),
            0,
            202,
            sunraycom::QosClass::kBestEffort,
            e2),
        true);

    asio::io_context io;
    asio::ip::udp::socket s1(io);
    asio::ip::udp::socket s2(io);

    std::error_code ec;
    s1.open(asio::ip::udp::v4(), ec);
    if (ec) {
        std::cerr << "socket create failed\n";
        return 2;
    }
    s2.open(asio::ip::udp::v4(), ec);
    if (ec) {
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

    s1.close(ec);
    s2.close(ec);
    runtime.event_bus().unsubscribe(tok);
    runtime.stop();

    if (got.load() < 2) {
        std::cerr << "source isolation decode failed, got=" << got.load() << "\n";
        return 3;
    }

    return 0;
}
