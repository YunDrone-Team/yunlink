/**
 * @file tests/test_transport_loop.cpp
 * @brief sunray_communication_lib source file.
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>

#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/runtime/runtime.hpp"

int main() {
    sunraycom::Runtime receiver;
    sunraycom::Runtime sender;

    sunraycom::RuntimeConfig rx_cfg;
    rx_cfg.udp_bind_port = 12096;
    rx_cfg.udp_target_port = 12096;
    rx_cfg.tcp_listen_port = 12196;
    rx_cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    rx_cfg.self_identity.agent_id = 91;
    rx_cfg.self_identity.role = sunraycom::EndpointRole::kObserver;

    sunraycom::RuntimeConfig tx_cfg;
    tx_cfg.udp_bind_port = 12097;
    tx_cfg.udp_target_port = 12097;
    tx_cfg.tcp_listen_port = 12197;
    tx_cfg.self_identity.agent_type = sunraycom::AgentType::kUav;
    tx_cfg.self_identity.agent_id = 1;
    tx_cfg.self_identity.role = sunraycom::EndpointRole::kVehicle;

    if (receiver.start(rx_cfg) != sunraycom::ErrorCode::kOk ||
        sender.start(tx_cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::atomic<bool> got{false};
    const size_t tok = receiver.state_subscriber().subscribe_vehicle_core(
        [&got](const sunraycom::TypedMessage<sunraycom::VehicleCoreState>& msg) {
            if (msg.payload.armed && msg.payload.x_m == 1.0F &&
                msg.payload.battery_percent == 87.5F) {
                got.store(true);
            }
        });

    std::string peer_id;
    if (sender.tcp_clients().connect_peer("127.0.0.1", rx_cfg.tcp_listen_port, &peer_id) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    sunraycom::VehicleCoreState state{};
    state.armed = true;
    state.nav_mode = 3;
    state.x_m = 1.0F;
    state.y_m = 2.0F;
    state.z_m = 3.0F;
    state.vx_mps = 0.1F;
    state.vy_mps = 0.2F;
    state.vz_mps = 0.3F;
    state.battery_percent = 87.5F;

    const auto ec = sender.publish_vehicle_core_state(
        peer_id,
        sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kGroundStation, 91),
        state);
    if (ec != sunraycom::ErrorCode::kOk) {
        std::cerr << "publish state failed\n";
        return 3;
    }

    for (int i = 0; i < 40 && !got.load(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    receiver.state_subscriber().unsubscribe(tok);
    sender.stop();
    receiver.stop();

    if (!got.load()) {
        std::cerr << "did not receive vehicle core state\n";
        return 4;
    }

    return 0;
}
