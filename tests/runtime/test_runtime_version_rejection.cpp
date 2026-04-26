/**
 * @file tests/runtime/test_runtime_version_rejection.cpp
 * @brief Runtime protocol/header/schema mismatch rejection tests.
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

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12600;
    air_cfg.udp_target_port = 12600;
    air_cfg.tcp_listen_port = 12700;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 10;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "version-runtime-secret";
    air_cfg.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12601;
    ground_cfg.udp_target_port = 12601;
    ground_cfg.tcp_listen_port = 12701;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 100;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "version-runtime-secret";

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
        ground.session_client().open_active_session(peer_id, "version-ground");
    const auto air_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 10);
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 100);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, air_target, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 3;
    }

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session_id, &air_session) ||
        air_session.peer.id.empty()) {
        std::cerr << "air session peer missing\n";
        return 4;
    }

    std::mutex mu;
    std::vector<yunlink::CommandResultView> results;
    std::vector<std::string> air_errors;
    std::vector<std::string> ground_errors;
    int goto_dispatch_count = 0;
    int state_count = 0;

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });
    const size_t air_error_token =
        air.event_bus().subscribe_error([&](const yunlink::ErrorEvent& ev) {
            std::lock_guard<std::mutex> lock(mu);
            air_errors.push_back(ev.message);
        });
    const size_t ground_error_token =
        ground.event_bus().subscribe_error([&](const yunlink::ErrorEvent& ev) {
            std::lock_guard<std::mutex> lock(mu);
            ground_errors.push_back(ev.message);
        });
    const size_t goto_token = air.command_subscriber().subscribe_goto(
        [&](const yunlink::InboundCommandView<yunlink::GotoCommand>&) {
            std::lock_guard<std::mutex> lock(mu);
            ++goto_dispatch_count;
        });
    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>&) {
            std::lock_guard<std::mutex> lock(mu);
            ++state_count;
        });

    yunlink::GotoCommand goto_cmd{};
    auto bad_schema_command = yunlink::make_typed_envelope(ground_cfg.self_identity,
                                                           air_target,
                                                           session_id,
                                                           900,
                                                           yunlink::QosClass::kReliableOrdered,
                                                           goto_cmd,
                                                           1000);
    bad_schema_command.message_id = 900;
    bad_schema_command.correlation_id = 900;
    bad_schema_command.schema_version = 99;

    yunlink::EnvelopeEvent bad_schema_event{};
    bad_schema_event.transport = yunlink::TransportType::kTcpServer;
    bad_schema_event.peer = air_session.peer;
    bad_schema_event.envelope = bad_schema_command;
    air.event_bus().publish_envelope(bad_schema_event);

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& result : results) {
                if (result.envelope.correlation_id == 900 &&
                    result.payload.phase == yunlink::CommandPhase::kFailed &&
                    result.payload.detail == "runtime-schema-version-mismatch") {
                    return true;
                }
            }
            return false;
        })) {
        std::cerr << "schema-mismatched command did not produce stable failure\n";
        return 5;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (goto_dispatch_count != 0) {
            std::cerr << "schema-mismatched command reached handler\n";
            return 6;
        }
    }

    yunlink::VehicleCoreState state{};
    state.battery_percent = 44.0F;
    auto bad_protocol_state = yunlink::make_typed_envelope(air_cfg.self_identity,
                                                           ground_target,
                                                           session_id,
                                                           0,
                                                           yunlink::QosClass::kReliableLatest,
                                                           state,
                                                           1000);
    bad_protocol_state.message_id = 901;
    bad_protocol_state.protocol_major = 2;

    yunlink::EnvelopeEvent bad_protocol_event{};
    bad_protocol_event.transport = yunlink::TransportType::kTcpClient;
    bad_protocol_event.peer.id = peer_id;
    bad_protocol_event.envelope = bad_protocol_state;
    ground.event_bus().publish_envelope(bad_protocol_event);

    {
        std::lock_guard<std::mutex> lock(mu);
        bool saw_protocol_error = false;
        for (const auto& error : ground_errors) {
            if (error == "runtime-protocol-version-mismatch") {
                saw_protocol_error = true;
            }
        }
        if (!saw_protocol_error || state_count != 0) {
            std::cerr << "protocol-mismatched state was not rejected before handler\n";
            return 7;
        }
    }

    auto bad_header_state = bad_protocol_state;
    bad_header_state.message_id = 902;
    bad_header_state.protocol_major = 1;
    bad_header_state.header_version = 2;
    yunlink::EnvelopeEvent bad_header_event = bad_protocol_event;
    bad_header_event.envelope = bad_header_state;
    ground.event_bus().publish_envelope(bad_header_event);

    {
        std::lock_guard<std::mutex> lock(mu);
        bool saw_header_error = false;
        for (const auto& error : ground_errors) {
            if (error == "runtime-protocol-version-mismatch") {
                saw_header_error = true;
            }
        }
        if (!saw_header_error || state_count != 0) {
            std::cerr << "header-mismatched state was not rejected before handler\n";
            return 8;
        }
    }

    air.command_subscriber().unsubscribe(goto_token);
    ground.state_subscriber().unsubscribe(state_token);
    air.event_bus().unsubscribe(air_error_token);
    ground.event_bus().unsubscribe(ground_error_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
