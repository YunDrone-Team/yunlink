/**
 * @file tests/security/test_security_tags.cpp
 * @brief Security auth tag, replay and key epoch runtime tests.
 */

#include <chrono>
#include <functional>
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

bool has_error(const std::vector<std::string>& errors, const std::string& expected) {
    for (const auto& error : errors) {
        if (error == expected) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12620;
    air_cfg.udp_target_port = 12620;
    air_cfg.tcp_listen_port = 12720;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 12;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "security-tags-secret";
    air_cfg.capability_flags = yunlink::kCapabilitySecurityTags;
    air_cfg.security_key_epoch = 7;
    air_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12621;
    ground_cfg.udp_target_port = 12621;
    ground_cfg.tcp_listen_port = 12721;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 120;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "security-tags-secret";
    ground_cfg.capability_flags = yunlink::kCapabilitySecurityTags;
    ground_cfg.security_key_epoch = 7;

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 2;
    }

    const uint64_t session_id =
        ground.session_client().open_active_session(peer_id, "secure-ground");
    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 12);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, target, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "secure session/authority setup failed\n";
        return 3;
    }
    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air.current_authority_for_target(target, &lease); })) {
        std::cerr << "secure authority not granted\n";
        return 4;
    }

    std::mutex mu;
    std::vector<std::string> air_errors;
    std::vector<yunlink::CommandResultView> results;
    int dispatch_count = 0;
    bool captured_command = false;
    yunlink::EnvelopeEvent captured{};

    const size_t error_token = air.event_bus().subscribe_error([&](const yunlink::ErrorEvent& ev) {
        std::lock_guard<std::mutex> lock(mu);
        air_errors.push_back(ev.message);
    });
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });
    const size_t raw_token =
        air.event_bus().subscribe_envelope([&](const yunlink::EnvelopeEvent& ev) {
            if (ev.envelope.message_family == yunlink::MessageFamily::kCommand &&
                ev.envelope.message_type ==
                    yunlink::MessageTraits<yunlink::GotoCommand>::kMessageType) {
                std::lock_guard<std::mutex> lock(mu);
                captured = ev;
                captured_command = true;
            }
        });
    const size_t cmd_token = air.command_subscriber().subscribe_goto(
        [&](const yunlink::InboundCommandView<yunlink::GotoCommand>& view) {
            {
                std::lock_guard<std::mutex> lock(mu);
                ++dispatch_count;
            }
            yunlink::CommandResult result{};
            result.command_kind = yunlink::CommandKind::kGoto;
            result.phase = yunlink::CommandPhase::kSucceeded;
            result.progress_percent = 100;
            result.detail = "secure-command-succeeded";
            (void)air.reply_command_result(view.inbound, result);
        });

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 1.0F;
    goto_cmd.y_m = 2.0F;
    goto_cmd.z_m = 3.0F;
    if (ground.command_publisher().publish_goto(peer_id, session_id, target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "secure goto publish failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return dispatch_count == 1 && captured_command &&
                   !captured.envelope.security.auth_tag.empty() && !results.empty() &&
                   results.back().payload.detail == "secure-command-succeeded";
        })) {
        std::cerr << "signed command did not complete\n";
        return 6;
    }

    air.event_bus().publish_envelope(captured);
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_error(air_errors, "security-replay-detected");
        })) {
        std::cerr << "replayed command was not rejected\n";
        return 7;
    }

    yunlink::EnvelopeEvent tampered = captured;
    tampered.envelope.message_id += 1000;
    tampered.envelope.correlation_id = tampered.envelope.message_id;
    tampered.envelope.payload.push_back(0x99);
    tampered.envelope.payload_len = static_cast<uint32_t>(tampered.envelope.payload.size());
    air.event_bus().publish_envelope(tampered);
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_error(air_errors, "security-auth-tag-mismatch");
        })) {
        std::cerr << "tampered command tag was not rejected\n";
        return 8;
    }

    yunlink::EnvelopeEvent old_epoch = captured;
    old_epoch.envelope.message_id += 2000;
    old_epoch.envelope.correlation_id = old_epoch.envelope.message_id;
    old_epoch.envelope.security.key_epoch = 6;
    air.event_bus().publish_envelope(old_epoch);
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return has_error(air_errors, "security-key-epoch-mismatch");
        })) {
        std::cerr << "old key epoch was not rejected\n";
        return 9;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (dispatch_count != 1) {
            std::cerr << "rejected security envelopes reached command handler\n";
            return 10;
        }
    }

    air.command_subscriber().unsubscribe(cmd_token);
    air.event_bus().unsubscribe(raw_token);
    ground.event_subscriber().unsubscribe(result_token);
    air.event_bus().unsubscribe(error_token);
    ground.stop();
    air.stop();
    return 0;
}
