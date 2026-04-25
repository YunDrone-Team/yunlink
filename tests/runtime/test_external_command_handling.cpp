/**
 * @file tests/runtime/test_external_command_handling.cpp
 * @brief External command handling mode regression test.
 */

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 160, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime client;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12360;
    server_cfg.udp_target_port = 12360;
    server_cfg.tcp_listen_port = 12460;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 9;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "external-command-secret";
    server_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig client_cfg;
    client_cfg.udp_bind_port = 12361;
    client_cfg.udp_target_port = 12361;
    client_cfg.tcp_listen_port = 12461;
    client_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    client_cfg.self_identity.agent_id = 42;
    client_cfg.self_identity.role = yunlink::EndpointRole::kController;
    client_cfg.shared_secret = "external-command-secret";

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        client.start(client_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "tcp connect failed\n";
        return 2;
    }

    const uint64_t session_id = client.session_client().open_active_session(peer_id, "bridge-test");
    if (session_id == 0) {
        std::cerr << "open session failed\n";
        return 3;
    }
    if (!wait_until([&]() { return server.session_server().has_active_session(session_id); })) {
        std::cerr << "session did not become active\n";
        return 4;
    }
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9);
    if (client.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 2000) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "authority request failed\n";
        return 5;
    }
    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return server.current_authority_for_target(target, &lease); })) {
        std::cerr << "authority not granted\n";
        return 6;
    }

    std::atomic<bool> saw_command{false};
    std::atomic<uint8_t> inbound_transport{0};
    std::atomic<uint8_t> outbound_transport{0};
    std::string inbound_peer_id;
    uint64_t inbound_session_id = 0;
    uint64_t inbound_message_id = 0;
    uint64_t inbound_correlation_id = 0;
    yunlink::GotoCommand observed_payload{};
    std::mutex mu;
    std::vector<yunlink::CommandResultView> results;

    const size_t cmd_token = server.command_subscriber().subscribe_goto(
        [&](const yunlink::InboundCommandView<yunlink::GotoCommand>& view) {
            saw_command.store(true);
            inbound_transport.store(static_cast<uint8_t>(view.inbound.transport));

            {
                std::lock_guard<std::mutex> lock(mu);
                inbound_peer_id = view.inbound.peer.id;
                inbound_session_id = view.inbound.envelope.session_id;
                inbound_message_id = view.inbound.envelope.message_id;
                inbound_correlation_id = view.inbound.envelope.correlation_id;
                observed_payload = view.payload;
            }

            yunlink::CommandResult result{};
            result.command_kind = yunlink::CommandKind::kGoto;
            result.phase = yunlink::CommandPhase::kReceived;
            result.progress_percent = 1;
            result.detail = "manual-bridge-result";
            if (server.reply_command_result(view.inbound, result) != yunlink::ErrorCode::kOk) {
                std::cerr << "reply_command_result failed in callback\n";
            }
        });

    const size_t raw_result_token =
        client.event_bus().subscribe_envelope([&](const yunlink::EnvelopeEvent& ev) {
            if (ev.envelope.message_family == yunlink::MessageFamily::kCommandResult) {
                outbound_transport.store(static_cast<uint8_t>(ev.transport));
            }
        });

    const size_t result_token = client.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 8.5F;
    goto_cmd.y_m = -2.0F;
    goto_cmd.z_m = 4.25F;
    goto_cmd.yaw_rad = 0.3F;

    yunlink::CommandHandle handle{};
    if (client.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd, &handle) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish goto failed\n";
        return 7;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return saw_command.load() && results.size() == 1;
        })) {
        std::cerr << "external command handling path not observed\n";
        return 8;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    server.command_subscriber().unsubscribe(cmd_token);
    client.event_bus().unsubscribe(raw_result_token);
    client.event_subscriber().unsubscribe(result_token);

    client.stop();
    server.stop();

    std::lock_guard<std::mutex> lock(mu);
    if (!saw_command.load()) {
        std::cerr << "command callback not invoked\n";
        return 9;
    }
    if (inbound_transport.load() != static_cast<uint8_t>(yunlink::TransportType::kTcpServer)) {
        std::cerr << "unexpected inbound transport\n";
        return 10;
    }
    if (outbound_transport.load() != static_cast<uint8_t>(yunlink::TransportType::kTcpClient)) {
        std::cerr << "unexpected outbound transport\n";
        return 11;
    }
    if (inbound_peer_id.empty() || inbound_session_id != session_id ||
        inbound_correlation_id != handle.message_id) {
        std::cerr << "callback route/envelope metadata mismatch\n";
        return 12;
    }
    if (observed_payload.x_m != goto_cmd.x_m || observed_payload.y_m != goto_cmd.y_m ||
        observed_payload.z_m != goto_cmd.z_m || observed_payload.yaw_rad != goto_cmd.yaw_rad) {
        std::cerr << "decoded goto payload mismatch\n";
        return 13;
    }
    if (results.size() != 1) {
        std::cerr << "unexpected number of command results in external mode\n";
        return 14;
    }

    const auto& result = results.front();
    if (result.payload.detail != "manual-bridge-result" ||
        result.payload.phase != yunlink::CommandPhase::kReceived ||
        result.envelope.session_id != session_id ||
        result.envelope.correlation_id != inbound_message_id ||
        result.envelope.correlation_id != handle.message_id) {
        std::cerr << "explicit command result reply metadata mismatch\n";
        return 15;
    }
    if (result.payload.detail == "runtime-auto-result") {
        std::cerr << "runtime auto result leaked in external mode\n";
        return 16;
    }

    return 0;
}
