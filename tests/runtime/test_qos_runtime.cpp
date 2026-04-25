/**
 * @file tests/runtime/test_qos_runtime.cpp
 * @brief Runtime QoS policy tests for command, state, event and bulk descriptor planes.
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
    air_cfg.udp_bind_port = 12590;
    air_cfg.udp_target_port = 12590;
    air_cfg.tcp_listen_port = 12690;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 9;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "qos-runtime-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12591;
    ground_cfg.udp_target_port = 12591;
    ground_cfg.tcp_listen_port = 12691;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 90;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "qos-runtime-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "qos-ground");
    const auto air_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 9);
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 90);
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
    std::vector<float> batteries;
    std::vector<std::string> errors;
    std::vector<yunlink::QosClass> raw_event_qos;

    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            std::lock_guard<std::mutex> lock(mu);
            results.push_back(view);
        });
    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            std::lock_guard<std::mutex> lock(mu);
            batteries.push_back(message.payload.battery_percent);
        });
    const size_t error_token = ground.event_bus().subscribe_error([&](const yunlink::ErrorEvent& ev) {
        std::lock_guard<std::mutex> lock(mu);
        errors.push_back(ev.message);
    });
    const size_t raw_token = ground.event_bus().subscribe_envelope(
        [&](const yunlink::EnvelopeEvent& ev) {
            if (ev.envelope.message_family == yunlink::MessageFamily::kStateEvent) {
                std::lock_guard<std::mutex> lock(mu);
                raw_event_qos.push_back(ev.envelope.qos_class);
            }
        });

    yunlink::GotoCommand goto_cmd{};
    auto bad_qos_command = yunlink::make_typed_envelope(ground_cfg.self_identity,
                                                        air_target,
                                                        session_id,
                                                        777,
                                                        yunlink::QosClass::kBestEffort,
                                                        goto_cmd,
                                                        1000);
    bad_qos_command.message_id = 777;
    bad_qos_command.correlation_id = 777;

    yunlink::EnvelopeEvent bad_qos_event{};
    bad_qos_event.transport = yunlink::TransportType::kTcpServer;
    bad_qos_event.peer = air_session.peer;
    bad_qos_event.envelope = bad_qos_command;
    air.event_bus().publish_envelope(bad_qos_event);

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            for (const auto& result : results) {
                if (result.envelope.correlation_id == 777 &&
                    result.payload.phase == yunlink::CommandPhase::kFailed &&
                    result.payload.detail == "command-qos-requires-reliable-ordered") {
                    return true;
                }
            }
            return false;
        })) {
        std::cerr << "best-effort command was not rejected by QoS policy\n";
        return 5;
    }

    yunlink::VehicleCoreState newest{};
    newest.battery_percent = 99.0F;
    auto newest_env = yunlink::make_typed_envelope(air_cfg.self_identity,
                                                   ground_target,
                                                   session_id,
                                                   0,
                                                   yunlink::QosClass::kReliableLatest,
                                                   newest,
                                                   1000);
    newest_env.message_id = 100;
    yunlink::EnvelopeEvent newest_event{};
    newest_event.transport = yunlink::TransportType::kTcpClient;
    newest_event.peer.id = peer_id;
    newest_event.envelope = newest_env;
    ground.event_bus().publish_envelope(newest_event);

    yunlink::VehicleCoreState older{};
    older.battery_percent = 12.0F;
    auto older_env = yunlink::make_typed_envelope(air_cfg.self_identity,
                                                  ground_target,
                                                  session_id,
                                                  0,
                                                  yunlink::QosClass::kReliableLatest,
                                                  older,
                                                  1000);
    older_env.message_id = 99;
    yunlink::EnvelopeEvent older_event = newest_event;
    older_event.envelope = older_env;
    ground.event_bus().publish_envelope(older_event);

    {
        std::lock_guard<std::mutex> lock(mu);
        if (batteries.size() != 1 || batteries.front() != 99.0F) {
            std::cerr << "reliable-latest state did not drop stale snapshot\n";
            return 6;
        }
    }

    yunlink::BulkChannelDescriptor bulk{};
    bulk.channel_id = 7;
    bulk.stream_type = yunlink::BulkStreamType::kPointCloud;
    bulk.state = yunlink::BulkChannelState::kReady;
    bulk.uri = "udp://239.1.1.7:5700";
    bulk.mtu_bytes = 1200;
    auto bad_bulk_qos = yunlink::make_typed_envelope(air_cfg.self_identity,
                                                     ground_target,
                                                     session_id,
                                                     0,
                                                     yunlink::QosClass::kBulk,
                                                     bulk,
                                                     1000);
    bad_bulk_qos.message_id = 200;
    yunlink::EnvelopeEvent bad_bulk_event = newest_event;
    bad_bulk_event.envelope = bad_bulk_qos;
    ground.event_bus().publish_envelope(bad_bulk_event);

    {
        std::lock_guard<std::mutex> lock(mu);
        bool saw_bulk_error = false;
        for (const auto& error : errors) {
            if (error == "bulk-descriptor-qos-requires-reliable-ordered") {
                saw_bulk_error = true;
            }
        }
        yunlink::BulkChannelDescriptor active{};
        if (!saw_bulk_error || ground.current_bulk_channel(7, &active)) {
            std::cerr << "bulk descriptor QoS policy mismatch\n";
            return 7;
        }
    }

    yunlink::VehicleEvent event{};
    event.kind = yunlink::VehicleEventKind::kInfo;
    event.detail = "best-effort-event";
    if (air.publish_vehicle_event(air_session.peer.id, ground_target, event, session_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "publish vehicle event failed\n";
        return 8;
    }
    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return !raw_event_qos.empty() &&
                   raw_event_qos.back() == yunlink::QosClass::kBestEffort;
        })) {
        std::cerr << "state event did not use best-effort QoS\n";
        return 9;
    }

    ground.event_bus().unsubscribe(raw_token);
    ground.event_bus().unsubscribe(error_token);
    ground.state_subscriber().unsubscribe(state_token);
    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
