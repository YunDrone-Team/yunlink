/**
 * @file tests/test_runtime_control_paths.cpp
 * @brief Runtime 语义路径回归测试。
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include "yunlink/core/semantic_messages.hpp"
#include "yunlink/runtime/runtime.hpp"

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

yunlink::SecureEnvelope make_authority_envelope(const yunlink::RuntimeConfig& cfg,
                                                  const yunlink::TargetSelector& target,
                                                  uint64_t session_id,
                                                  yunlink::AuthorityAction action,
                                                  uint32_t lease_ttl_ms) {
    yunlink::AuthorityRequest payload{};
    payload.action = action;
    payload.source = yunlink::ControlSource::kGroundStation;
    payload.lease_ttl_ms = lease_ttl_ms;
    payload.allow_preempt = action == yunlink::AuthorityAction::kPreempt;

    auto envelope = yunlink::make_typed_envelope(cfg.self_identity,
                                                   target,
                                                   session_id,
                                                   0,
                                                   yunlink::QosClass::kReliableOrdered,
                                                   payload,
                                                   lease_ttl_ms);
    static uint64_t next_message_id = 50000;
    envelope.message_id = next_message_id++;
    envelope.correlation_id = envelope.message_id;
    return envelope;
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client_a;
    yunlink::Runtime client_b;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12330;
    server_cfg.udp_target_port = 12330;
    server_cfg.tcp_listen_port = 12430;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig client_a_cfg;
    client_a_cfg.udp_bind_port = 12331;
    client_a_cfg.udp_target_port = 12331;
    client_a_cfg.tcp_listen_port = 12431;
    client_a_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_a_cfg.self_identity.agent_id = 7;
    client_a_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_a_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig client_b_cfg = client_a_cfg;
    client_b_cfg.udp_bind_port = 12332;
    client_b_cfg.udp_target_port = 12332;
    client_b_cfg.tcp_listen_port = 12432;
    client_b_cfg.self_identity.agent_id = 8;

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client_a.start(client_a_cfg) != yunlink::ErrorCode::kOk ||
        client_b.start(client_b_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_a;
    std::string peer_b;
    if (client_a.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_a) !=
            yunlink::ErrorCode::kOk ||
        client_b.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_b) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_a = client_a.session_client().open_active_session(peer_a, "station-a");
    const uint64_t session_b_unused =
        client_b.session_client().open_active_session(peer_b, "station-b-unused");
    const uint64_t session_b = client_b.session_client().open_active_session(peer_b, "station-b");
    if (session_a == 0 || session_b_unused == 0 || session_b == 0) {
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

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    if (client_a.request_authority(
            peer_a, session_a, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority claim failed\n";
        return 5;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() {
            return server.current_authority(&lease) &&
                   lease.state == yunlink::AuthorityState::kController &&
                   lease.session_id == session_a;
        })) {
        std::cerr << "authority claim not applied\n";
        return 6;
    }
    const auto renew = make_authority_envelope(
        client_a_cfg, target, session_a, yunlink::AuthorityAction::kRenew, 5000);
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

    if (client_a.release_authority(peer_a, session_a, target) != yunlink::ErrorCode::kOk) {
        std::cerr << "authority release send failed\n";
        return 9;
    }
    if (!wait_until([&]() { return !server.current_authority(&lease); })) {
        std::cerr << "authority release not applied\n";
        return 10;
    }

    if (client_a.request_authority(
            peer_a, session_a, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority reclaim failed\n";
        return 11;
    }
    if (!wait_until(
            [&]() { return server.current_authority(&lease) && lease.session_id == session_a; })) {
        std::cerr << "authority reclaim not applied\n";
        return 12;
    }
    const std::string authority_peer_before_preempt = lease.peer.id;

    if (client_b.request_authority(
            peer_b, session_b, target, yunlink::ControlSource::kGroundStation, 1500, false) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority non-preempt send failed\n";
        return 13;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
    if (!server.current_authority(&lease) || lease.session_id != session_a ||
        lease.peer.id != authority_peer_before_preempt) {
        std::cerr << "authority holder changed without preempt\n";
        return 14;
    }

    if (client_b.request_authority(
            peer_b, session_b, target, yunlink::ControlSource::kGroundStation, 3000, true) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority preempt send failed\n";
        return 15;
    }
    if (!wait_until([&]() {
            return server.current_authority(&lease) && lease.session_id == session_b &&
                   lease.lease_ttl_ms == 3000 && lease.peer.id != authority_peer_before_preempt;
        })) {
        std::cerr << "authority preempt not applied\n";
        return 16;
    }

    std::atomic<bool> saw_server_command{false};
    std::atomic<int> raw_result_count{0};
    yunlink::SecureEnvelope observed_command{};
    std::vector<yunlink::CommandPhase> phases;

    const size_t raw_server_token =
        server.event_bus().subscribe_envelope([&](const yunlink::EnvelopeEvent& ev) {
            if (ev.transport == yunlink::TransportType::kTcpServer &&
                ev.envelope.message_family == yunlink::MessageFamily::kCommand &&
                ev.envelope.message_type ==
                    yunlink::MessageTraits<yunlink::GotoCommand>::kMessageType) {
                observed_command = ev.envelope;
                saw_server_command.store(true);
            }
        });

    const size_t raw_client_token =
        client_b.event_bus().subscribe_envelope([&](const yunlink::EnvelopeEvent& ev) {
            if (ev.transport == yunlink::TransportType::kTcpClient &&
                ev.envelope.message_family == yunlink::MessageFamily::kCommandResult) {
                ++raw_result_count;
            }
        });

    const size_t result_token = client_b.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) { phases.push_back(view.payload.phase); });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 5.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 3.0F;
    goto_cmd.yaw_rad = 0.2F;

    yunlink::CommandHandle handle{};
    if (client_b.command_publisher().publish_goto(peer_b, session_b, target, goto_cmd, &handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish goto failed\n";
        return 17;
    }

    if (!wait_until([&]() {
            return saw_server_command.load() && raw_result_count.load() >= 4 && phases.size() >= 4;
        })) {
        std::cerr << "command path not observed\n";
        return 18;
    }

    server.event_bus().unsubscribe(raw_server_token);
    client_b.event_bus().unsubscribe(raw_client_token);
    client_b.event_subscriber().unsubscribe(result_token);

    client_b.stop();
    client_a.stop();
    server.stop();

    if (!saw_server_command.load() ||
        observed_command.message_family != yunlink::MessageFamily::kCommand ||
        observed_command.message_type !=
            yunlink::MessageTraits<yunlink::GotoCommand>::kMessageType ||
        observed_command.message_id == 0 ||
        observed_command.correlation_id != observed_command.message_id ||
        observed_command.session_id != session_b) {
        std::cerr << "command helper metadata mismatch\n";
        return 19;
    }

    const std::vector<yunlink::CommandPhase> expected = {
        yunlink::CommandPhase::kReceived,
        yunlink::CommandPhase::kAccepted,
        yunlink::CommandPhase::kInProgress,
        yunlink::CommandPhase::kSucceeded,
    };
    if (phases.size() < expected.size() ||
        !std::equal(expected.begin(), expected.end(), phases.begin())) {
        std::cerr << "command phase flow mismatch\n";
        return 20;
    }

    if (raw_result_count.load() < 4) {
        std::cerr << "client raw result stream mismatch\n";
        return 21;
    }

    return 0;
}
