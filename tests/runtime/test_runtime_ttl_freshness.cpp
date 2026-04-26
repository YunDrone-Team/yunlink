/**
 * @file tests/runtime/test_runtime_ttl_freshness.cpp
 * @brief Runtime TTL freshness rejection tests.
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "yunlink/core/semantic_message_codec.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 100, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

uint64_t now_ms() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12370;
    server_cfg.udp_target_port = 12370;
    server_cfg.tcp_listen_port = 12470;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 9;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "ttl-freshness-secret";
    server_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12371;
    client_cfg.udp_target_port = 12371;
    client_cfg.tcp_listen_port = 12471;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 42;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_cfg.shared_secret = "ttl-freshness-secret";

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::atomic<bool> handler_called{false};
    const size_t cmd_token = server.command_subscriber().subscribe_goto(
        [&](const yunlink::InboundCommandView<yunlink::GotoCommand>&) {
            handler_called.store(true);
        });

    std::mutex mu;
    std::vector<yunlink::CommandResultView> results;
    const size_t result_token = client.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });

    yunlink::GotoCommand payload{};
    payload.x_m = 1.0F;
    payload.y_m = 2.0F;
    payload.z_m = 3.0F;
    payload.yaw_rad = 0.5F;

    auto expired = yunlink::make_typed_envelope(
        client_cfg.self_identity,
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9),
        777,
        0,
        yunlink::QosClass::kReliableOrdered,
        payload,
        1);
    expired.created_at_ms = now_ms() - 10'000;
    expired.message_id = 987654321;
    expired.correlation_id = expired.message_id;

    if (client.udp().send_envelope_unicast(expired, "127.0.0.1", server_cfg.udp_bind_port) < 0) {
        std::cerr << "failed to send expired envelope\n";
        return 2;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return !results.empty();
        })) {
        std::cerr << "expired command did not produce a command result\n";
        return 3;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    server.command_subscriber().unsubscribe(cmd_token);
    client.event_subscriber().unsubscribe(result_token);
    client.stop();
    server.stop();

    if (handler_called.load()) {
        std::cerr << "expired command reached command handler\n";
        return 4;
    }

    std::lock_guard<std::mutex> lock(mu);
    if (results.size() != 1) {
        std::cerr << "unexpected expired command result count: " << results.size() << "\n";
        return 5;
    }
    const auto& result = results.front();
    if (result.payload.command_kind != yunlink::CommandKind::kGoto ||
        result.payload.phase != yunlink::CommandPhase::kExpired ||
        result.payload.detail != "runtime-ttl-expired" ||
        result.envelope.session_id != expired.session_id ||
        result.envelope.correlation_id != expired.message_id) {
        std::cerr << "expired command result metadata mismatch\n";
        return 6;
    }

    return 0;
}
