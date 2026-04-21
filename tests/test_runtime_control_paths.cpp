/**
 * @file tests/test_runtime_control_paths.cpp
 * @brief Runtime 语义路径回归测试。
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "sunraycom/core/semantic_messages.hpp"
#include "sunraycom/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 120, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

sunraycom::SecureEnvelope make_authority_envelope(const sunraycom::RuntimeConfig& cfg,
                                                  const sunraycom::TargetSelector& target,
                                                  uint64_t session_id,
                                                  sunraycom::AuthorityAction action,
                                                  uint32_t lease_ttl_ms) {
    sunraycom::AuthorityRequest payload{};
    payload.action = action;
    payload.source = sunraycom::ControlSource::kGroundStation;
    payload.lease_ttl_ms = lease_ttl_ms;
    payload.allow_preempt = action == sunraycom::AuthorityAction::kPreempt;

    auto envelope = sunraycom::make_typed_envelope(cfg.self_identity,
                                                   target,
                                                   session_id,
                                                   0,
                                                   sunraycom::QosClass::kReliableOrdered,
                                                   payload,
                                                   lease_ttl_ms);
    static uint64_t next_message_id = 50000;
    envelope.message_id = next_message_id++;
    envelope.correlation_id = envelope.message_id;
    return envelope;
}

}  // namespace

int main() {
    sunraycom::Runtime server;
    sunraycom::Runtime client_a;
    sunraycom::Runtime client_b;

    sunraycom::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12330;
    server_cfg.udp_target_port = 12330;
    server_cfg.tcp_listen_port = 12430;
    server_cfg.self_identity.agent_type = sunraycom::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = sunraycom::EndpointRole::kVehicle;
    server_cfg.shared_secret = "sunray-secret";

    sunraycom::RuntimeConfig client_a_cfg;
    client_a_cfg.udp_bind_port = 12331;
    client_a_cfg.udp_target_port = 12331;
    client_a_cfg.tcp_listen_port = 12431;
    client_a_cfg.self_identity.agent_type = sunraycom::AgentType::kGroundStation;
    client_a_cfg.self_identity.agent_id = 7;
    client_a_cfg.self_identity.role = sunraycom::EndpointRole::kController;
    client_a_cfg.shared_secret = "sunray-secret";

    sunraycom::RuntimeConfig client_b_cfg = client_a_cfg;
    client_b_cfg.udp_bind_port = 12332;
    client_b_cfg.udp_target_port = 12332;
    client_b_cfg.tcp_listen_port = 12432;
    client_b_cfg.self_identity.agent_id = 8;

    if (server.start(server_cfg) != sunraycom::ErrorCode::kOk ||
        client_a.start(client_a_cfg) != sunraycom::ErrorCode::kOk ||
        client_b.start(client_b_cfg) != sunraycom::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_a;
    std::string peer_b;
    if (client_a.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_a) !=
            sunraycom::ErrorCode::kOk ||
        client_b.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_b) !=
            sunraycom::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_a = client_a.session_client().open_active_session(peer_a, "station-a");
    const uint64_t session_b = client_b.session_client().open_active_session(peer_b, "station-b");
    if (session_a == 0 || session_b == 0) {
        std::cerr << "open session failed\n";
        return 3;
    }

    if (!wait_until([&]() {
            return server.session_server().has_active_session(session_a) &&
                   server.session_server().has_active_session(session_b);
        })) {
        std::cerr << "sessions did not become active\n";
        return 4;
    }

    const auto target = sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kUav, 1);
    if (client_a.request_authority(
            peer_a, session_a, target, sunraycom::ControlSource::kGroundStation, 2000) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "authority claim failed\n";
        return 5;
    }

    sunraycom::AuthorityLease lease{};
    if (!wait_until([&]() {
            return server.current_authority(&lease) &&
                   lease.state == sunraycom::AuthorityState::kController &&
                   lease.session_id == session_a;
        })) {
        std::cerr << "authority claim not applied\n";
        return 6;
    }

    const auto renew = make_authority_envelope(
        client_a_cfg, target, session_a, sunraycom::AuthorityAction::kRenew, 5000);
    if (client_a.tcp_clients().send_envelope(peer_a, renew) < 0) {
        std::cerr << "authority renew send failed\n";
        return 7;
    }
    if (!wait_until([&]() {
            return server.current_authority(&lease) && lease.session_id == session_a &&
                   lease.lease_ttl_ms == 5000;
        })) {
        std::cerr << "authority renew not applied\n";
        return 8;
    }

    if (client_a.release_authority(peer_a, session_a, target) != sunraycom::ErrorCode::kOk) {
        std::cerr << "authority release send failed\n";
        return 9;
    }
    if (!wait_until([&]() { return !server.current_authority(&lease); })) {
        std::cerr << "authority release not applied\n";
        return 10;
    }

    if (client_a.request_authority(
            peer_a, session_a, target, sunraycom::ControlSource::kGroundStation, 2000) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "authority reclaim failed\n";
        return 11;
    }
    if (!wait_until(
            [&]() { return server.current_authority(&lease) && lease.session_id == session_a; })) {
        std::cerr << "authority reclaim not applied\n";
        return 12;
    }

    if (client_b.request_authority(
            peer_b, session_b, target, sunraycom::ControlSource::kGroundStation, 3000, true) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "authority preempt send failed\n";
        return 13;
    }
    if (!wait_until([&]() {
            return server.current_authority(&lease) && lease.session_id == session_b &&
                   lease.lease_ttl_ms == 3000;
        })) {
        std::cerr << "authority preempt not applied\n";
        return 14;
    }

    std::atomic<bool> saw_server_command{false};
    std::atomic<int> raw_result_count{0};
    sunraycom::SecureEnvelope observed_command{};
    std::vector<sunraycom::CommandPhase> phases;

    const size_t raw_server_token =
        server.event_bus().subscribe_envelope([&](const sunraycom::EnvelopeEvent& ev) {
            if (ev.transport == sunraycom::TransportType::kTcpServer &&
                ev.envelope.message_family == sunraycom::MessageFamily::kCommand &&
                ev.envelope.message_type ==
                    sunraycom::MessageTraits<sunraycom::GotoCommand>::kMessageType) {
                observed_command = ev.envelope;
                saw_server_command.store(true);
            }
        });

    const size_t raw_client_token =
        client_b.event_bus().subscribe_envelope([&](const sunraycom::EnvelopeEvent& ev) {
            if (ev.transport == sunraycom::TransportType::kTcpClient &&
                ev.envelope.message_family == sunraycom::MessageFamily::kCommandResult) {
                ++raw_result_count;
            }
        });

    const size_t result_token = client_b.event_subscriber().subscribe_command_results(
        [&](const sunraycom::CommandResultView& view) { phases.push_back(view.payload.phase); });

    sunraycom::GotoCommand goto_cmd{};
    goto_cmd.x_m = 5.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 3.0F;
    goto_cmd.yaw_rad = 0.2F;

    sunraycom::CommandHandle handle{};
    if (client_b.command_publisher().publish_goto(peer_b, session_b, target, goto_cmd, &handle) !=
        sunraycom::ErrorCode::kOk) {
        std::cerr << "publish goto failed\n";
        return 15;
    }

    if (!wait_until([&]() {
            return saw_server_command.load() && raw_result_count.load() >= 4 && phases.size() >= 4;
        })) {
        std::cerr << "command path not observed\n";
        return 16;
    }

    server.event_bus().unsubscribe(raw_server_token);
    client_b.event_bus().unsubscribe(raw_client_token);
    client_b.event_subscriber().unsubscribe(result_token);

    client_b.stop();
    client_a.stop();
    server.stop();

    if (!saw_server_command.load() ||
        observed_command.message_family != sunraycom::MessageFamily::kCommand ||
        observed_command.message_type !=
            sunraycom::MessageTraits<sunraycom::GotoCommand>::kMessageType ||
        observed_command.message_id == 0 ||
        observed_command.correlation_id != observed_command.message_id ||
        observed_command.session_id != session_b) {
        std::cerr << "command helper metadata mismatch\n";
        return 17;
    }

    const std::vector<sunraycom::CommandPhase> expected = {
        sunraycom::CommandPhase::kReceived,
        sunraycom::CommandPhase::kAccepted,
        sunraycom::CommandPhase::kInProgress,
        sunraycom::CommandPhase::kSucceeded,
    };
    if (phases.size() < expected.size() ||
        !std::equal(expected.begin(), expected.end(), phases.begin())) {
        std::cerr << "command phase flow mismatch\n";
        return 18;
    }

    if (raw_result_count.load() < 4) {
        std::cerr << "client raw result stream mismatch\n";
        return 19;
    }

    return 0;
}
