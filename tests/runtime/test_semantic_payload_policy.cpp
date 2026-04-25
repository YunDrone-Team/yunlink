/**
 * @file tests/runtime/test_semantic_payload_policy.cpp
 * @brief Semantic payload boundary, enum and malformed decode policy tests.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <mutex>
#include <string>
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

bool has_decode_error(const std::vector<std::string>& errors) {
    for (const auto& error : errors) {
        if (error == "semantic-payload-decode-failed") {
            return true;
        }
    }
    return false;
}

}  // namespace

int main() {
    yunlink::FormationTaskCommand long_label{};
    long_label.group_id = 1;
    long_label.label.assign(2000, 'x');
    const auto label_bytes = yunlink::encode_payload(long_label);
    yunlink::FormationTaskCommand decoded_label{};
    if (!yunlink::decode_typed_payload(label_bytes, &decoded_label)) {
        std::cerr << "long label decode failed instead of truncating\n";
        return 1;
    }
    if (decoded_label.label.size() != 1024) {
        std::cerr << "long label was not truncated to fixed semantic capacity\n";
        return 2;
    }

    yunlink::ByteBuffer unknown_event_kind = {
        0xFF,  // kind
        0x01,  // severity
        0x00,
        0x00,  // empty detail string
    };
    yunlink::VehicleEvent event{};
    if (yunlink::decode_typed_payload(unknown_event_kind, &event)) {
        std::cerr << "unknown VehicleEventKind decoded as valid\n";
        return 3;
    }

    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12610;
    air_cfg.udp_target_port = 12610;
    air_cfg.tcp_listen_port = 12710;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 11;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "semantic-policy-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12611;
    ground_cfg.udp_target_port = 12611;
    ground_cfg.tcp_listen_port = 12711;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 110;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "semantic-policy-secret";

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 4;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 5;
    }

    const uint64_t session_id =
        ground.session_client().open_active_session(peer_id, "semantic-ground");
    const auto air_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 11);
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 110);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, air_target, yunlink::ControlSource::kGroundStation, 2000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 6;
    }

    yunlink::SessionDescriptor air_session{};
    if (!air.session_server().describe_session(session_id, &air_session) ||
        air_session.peer.id.empty()) {
        std::cerr << "air session peer missing\n";
        return 7;
    }
    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air.current_authority_for_target(air_target, &lease); })) {
        std::cerr << "authority not granted\n";
        return 8;
    }

    std::mutex mu;
    std::vector<yunlink::CommandResultView> results;
    std::vector<std::string> ground_errors;
    int goto_dispatch_count = 0;
    int state_count = 0;

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });
    const size_t error_token =
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

    auto malformed_goto = yunlink::make_typed_envelope(ground_cfg.self_identity,
                                                       air_target,
                                                       session_id,
                                                       300,
                                                       yunlink::QosClass::kReliableOrdered,
                                                       yunlink::GotoCommand{},
                                                       1000);
    malformed_goto.message_id = 300;
    malformed_goto.correlation_id = 300;
    malformed_goto.payload = {0x01, 0x02};
    malformed_goto.payload_len = static_cast<uint32_t>(malformed_goto.payload.size());

    yunlink::EnvelopeEvent malformed_command_event{};
    malformed_command_event.transport = yunlink::TransportType::kTcpServer;
    malformed_command_event.peer = air_session.peer;
    malformed_command_event.envelope = malformed_goto;
    air.event_bus().publish_envelope(malformed_command_event);

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& result : results) {
                if (result.envelope.correlation_id == 300 &&
                    result.payload.phase == yunlink::CommandPhase::kFailed &&
                    result.payload.detail == "semantic-payload-decode-failed") {
                    return true;
                }
            }
            return false;
        })) {
        std::cerr << "malformed command did not produce stable failed result\n";
        return 9;
    }

    {
        std::lock_guard<std::mutex> lock(mu);
        if (goto_dispatch_count != 0) {
            std::cerr << "malformed command reached handler\n";
            return 10;
        }
    }

    auto malformed_state = yunlink::make_typed_envelope(air_cfg.self_identity,
                                                        ground_target,
                                                        session_id,
                                                        0,
                                                        yunlink::QosClass::kReliableLatest,
                                                        yunlink::VehicleCoreState{},
                                                        1000);
    malformed_state.message_id = 301;
    malformed_state.payload = {0x01, 0x02};
    malformed_state.payload_len = static_cast<uint32_t>(malformed_state.payload.size());

    yunlink::EnvelopeEvent malformed_state_event{};
    malformed_state_event.transport = yunlink::TransportType::kTcpClient;
    malformed_state_event.peer.id = peer_id;
    malformed_state_event.envelope = malformed_state;
    ground.event_bus().publish_envelope(malformed_state_event);

    {
        std::lock_guard<std::mutex> lock(mu);
        if (state_count != 0 || !has_decode_error(ground_errors)) {
            std::cerr << "malformed state did not publish stable decode error\n";
            return 11;
        }
    }

    air.command_subscriber().unsubscribe(goto_token);
    ground.state_subscriber().unsubscribe(state_token);
    ground.event_bus().unsubscribe(error_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
