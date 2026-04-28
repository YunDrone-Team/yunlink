/**
 * @file tests/runtime/test_state_plane_semantics.cpp
 * @brief State snapshot/event source identity and ordering tests.
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

struct OrderedMessage {
    uint64_t message_id = 0;
    uint32_t source_id = 0;
    uint8_t source_role = 0;
};

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12610;
    air_cfg.udp_target_port = 12610;
    air_cfg.tcp_listen_port = 12710;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 21;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "state-semantics-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12611;
    ground_cfg.udp_target_port = 12611;
    ground_cfg.tcp_listen_port = 12711;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 210;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "state-semantics-secret";

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

    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "state-order");
    const auto air_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 21);
    const auto ground_target =
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 210);
    if (session_id == 0 ||
        !wait_until([&]() { return air.session_server().has_active_session(session_id); }) ||
        ground.request_authority(
            peer_id, session_id, air_target, yunlink::ControlSource::kGroundStation, 3000) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "session/authority setup failed\n";
        return 3;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until([&]() { return air.current_authority_for_target(air_target, &lease); })) {
        std::cerr << "authority not granted\n";
        return 4;
    }

    std::mutex mu;
    std::vector<OrderedMessage> state_messages;
    std::vector<OrderedMessage> event_messages;

    const size_t state_token = ground.state_subscriber().subscribe_vehicle_core(
        [&](const yunlink::TypedMessage<yunlink::VehicleCoreState>& message) {
            std::lock_guard<std::mutex> lock(mu);
            state_messages.push_back(
                {message.envelope.message_id,
                 message.envelope.source.agent_id,
                 static_cast<uint8_t>(message.envelope.source.role)});
        });
    const size_t event_token = ground.event_subscriber().subscribe_vehicle_event(
        [&](const yunlink::TypedMessage<yunlink::VehicleEvent>& message) {
            std::lock_guard<std::mutex> lock(mu);
            event_messages.push_back(
                {message.envelope.message_id,
                 message.envelope.source.agent_id,
                 static_cast<uint8_t>(message.envelope.source.role)});
        });

    yunlink::VehicleCoreState first{};
    first.armed = true;
    first.nav_mode = 1;
    first.battery_percent = 75.0F;
    yunlink::VehicleCoreState second = first;
    second.nav_mode = 2;
    second.battery_percent = 74.0F;

    yunlink::VehicleEvent first_event{};
    first_event.kind = yunlink::VehicleEventKind::kInfo;
    first_event.detail = "first-state-event";
    yunlink::VehicleEvent second_event = first_event;
    second_event.detail = "second-state-event";

    if (air.publish_vehicle_core_state(lease.peer.id, ground_target, first, session_id) !=
            yunlink::ErrorCode::kOk ||
        air.publish_vehicle_core_state(lease.peer.id, ground_target, second, session_id) !=
            yunlink::ErrorCode::kOk ||
        air.publish_vehicle_event(lease.peer.id, ground_target, first_event, session_id) !=
            yunlink::ErrorCode::kOk ||
        air.publish_vehicle_event(lease.peer.id, ground_target, second_event, session_id) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "state/event publish failed\n";
        return 5;
    }

    if (!wait_until([&]() {
            std::lock_guard<std::mutex> lock(mu);
            return state_messages.size() >= 2 && event_messages.size() >= 2;
        })) {
        std::cerr << "did not observe ordered state/event deliveries\n";
        return 6;
    }

    ground.state_subscriber().unsubscribe(state_token);
    ground.event_subscriber().unsubscribe(event_token);
    ground.stop();
    air.stop();

    {
        std::lock_guard<std::mutex> lock(mu);
        if (state_messages[0].source_id != air_cfg.self_identity.agent_id ||
            state_messages[0].source_role != static_cast<uint8_t>(air_cfg.self_identity.role) ||
            event_messages[0].source_id != air_cfg.self_identity.agent_id ||
            event_messages[0].source_role != static_cast<uint8_t>(air_cfg.self_identity.role)) {
            std::cerr << "state/event source identity mismatch\n";
            return 7;
        }
        if (!(state_messages[0].message_id < state_messages[1].message_id) ||
            !(event_messages[0].message_id < event_messages[1].message_id)) {
            std::cerr << "state/event ordering mismatch\n";
            return 8;
        }
    }

    return 0;
}
