/** @file @brief Schema-1 UGV codec and runtime routing coverage. */

#include <atomic>
#include <chrono>
#include <cmath>
#include <functional>
#include <iostream>
#include <thread>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& predicate) {
    for (int attempt = 0; attempt < 250; ++attempt) {
        if (predicate()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

yunlink::EndpointIdentity identity(yunlink::AgentType type, uint32_t id) {
    return {type,
            id,
            type == yunlink::AgentType::kGroundStation ? yunlink::EndpointRole::kController
                                                       : yunlink::EndpointRole::kVehicle};
}

template <typename T> bool rejects_truncation(const T& payload) {
    auto bytes = yunlink::encode_payload(payload);
    if (bytes.empty()) {
        return false;
    }
    bytes.pop_back();
    T decoded{};
    return !yunlink::decode_payload(bytes, &decoded);
}

bool close(float lhs, float rhs) {
    return std::fabs(lhs - rhs) < 0.0001F;
}

}  // namespace

int main() {
    static_assert(yunlink::MessageTraits<yunlink::UgvControlCommand>::kSchemaVersion == 1);
    static_assert(yunlink::MessageTraits<yunlink::UgvControlCmdSnapshot>::kSchemaVersion == 1);
    static_assert(yunlink::MessageTraits<yunlink::UgvControlStateSnapshot>::kSchemaVersion == 1);

    yunlink::UgvControlCommand command{};
    command.control_cmd = 5;
    command.desired_position_m = {1.0F, 2.0F, 0.0F};
    command.desired_velocity_mps = {0.5F, -0.25F, 0.0F};
    command.body_linear_velocity_mps = {1.5F, 0.0F, 0.0F};
    command.body_angular_velocity_radps = {0.0F, 0.0F, -0.4F};
    command.desired_yaw_rad = 1.25F;
    command.desired_wgs84_latitude_deg = 22.5401;
    command.desired_wgs84_longitude_deg = 113.9345;
    command.desired_wgs84_altitude_m = 18.5;
    yunlink::UgvControlCommand decoded_command{};
    const auto command_bytes = yunlink::encode_payload(command);
    if (!yunlink::decode_payload(command_bytes, &decoded_command) ||
        decoded_command.control_cmd != command.control_cmd ||
        !close(decoded_command.body_angular_velocity_radps.z,
               command.body_angular_velocity_radps.z) ||
        decoded_command.desired_wgs84_longitude_deg != command.desired_wgs84_longitude_deg ||
        !rejects_truncation(command)) {
        std::cerr << "UGV command codec failed\n";
        return 1;
    }

    yunlink::UgvControlCmdSnapshot command_snapshot{};
    command_snapshot.header = {"map", 123456U};
    command_snapshot.cmd_source = 1;
    command_snapshot.control_cmd = command.control_cmd;
    command_snapshot.body_angular_velocity_radps = command.body_angular_velocity_radps;
    command_snapshot.desired_wgs84_position = {22.5401, 113.9345, 18.5};
    yunlink::UgvControlCmdSnapshot decoded_snapshot{};
    if (!yunlink::decode_payload(yunlink::encode_payload(command_snapshot), &decoded_snapshot) ||
        decoded_snapshot.header.frame_id != "map" || decoded_snapshot.cmd_source != 1 ||
        decoded_snapshot.desired_wgs84_position.latitude_deg != 22.5401 ||
        !rejects_truncation(command_snapshot)) {
        std::cerr << "UGV command snapshot codec failed\n";
        return 2;
    }

    yunlink::UgvControlStateSnapshot control_state{};
    control_state.header = {"odom", 654321U};
    control_state.agent_name = "ugv";
    control_state.agent_id = 3;
    control_state.drive_type = 2;
    control_state.control_cmd_valid = true;
    control_state.inside_geo_fence = true;
    control_state.diagnostic_level = 1;
    control_state.diagnostic_message = "lateral velocity ignored";
    control_state.fsm_state = 3;
    control_state.active_command = command_snapshot;
    control_state.self_odom.pose.position_m = {4.0F, 5.0F, 0.0F};
    control_state.odom_valid = true;
    control_state.target_valid = true;
    control_state.target_position_m = {8.0F, 9.0F, 0.0F};
    control_state.target_yaw_rad = 0.75F;
    control_state.controller_linear_velocity_mps = {0.7F, 0.0F, 0.0F};
    control_state.controller_angular_velocity_radps = {0.0F, 0.0F, 0.2F};
    control_state.geo_fence_min_m = {-10.0F, -10.0F, -1.0F};
    control_state.geo_fence_max_m = {10.0F, 10.0F, 1.0F};
    yunlink::UgvControlStateSnapshot decoded_state{};
    if (!yunlink::decode_payload(yunlink::encode_payload(control_state), &decoded_state) ||
        decoded_state.agent_name != "ugv" || decoded_state.agent_id != 3 ||
        decoded_state.diagnostic_message != control_state.diagnostic_message ||
        !close(decoded_state.controller_angular_velocity_radps.z, 0.2F) ||
        !rejects_truncation(control_state)) {
        std::cerr << "UGV control state codec failed\n";
        return 3;
    }

    yunlink::Runtime vehicle;
    yunlink::Runtime ground;
    yunlink::RuntimeConfig vehicle_config{};
    vehicle_config.udp_bind_port = 14630;
    vehicle_config.udp_target_port = 14630;
    vehicle_config.tcp_listen_port = 14730;
    vehicle_config.self_identity = identity(yunlink::AgentType::kUgv, 3);
    vehicle_config.shared_secret = "ugv-runtime-test";
    vehicle_config.command_handling_mode = yunlink::CommandHandlingMode::kExternalHandler;
    yunlink::RuntimeConfig ground_config{};
    ground_config.udp_bind_port = 14631;
    ground_config.udp_target_port = 14631;
    ground_config.tcp_listen_port = 14731;
    ground_config.self_identity = identity(yunlink::AgentType::kGroundStation, 30);
    ground_config.shared_secret = vehicle_config.shared_secret;
    if (vehicle.start(vehicle_config) != yunlink::ErrorCode::kOk ||
        ground.start(ground_config) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 4;
    }

    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", 14730, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "connect failed\n";
        return 5;
    }
    const uint64_t session_id = ground.session_client().open_active_session(peer_id, "ugv-test");
    if (session_id == 0 ||
        !wait_until([&] { return vehicle.session_server().has_active_session(session_id); })) {
        std::cerr << "session failed\n";
        return 6;
    }
    const auto ugv_target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUgv, 3);
    if (ground.request_authority(
            peer_id, session_id, ugv_target, yunlink::ControlSource::kGroundStation, 3000) !=
            yunlink::ErrorCode::kOk ||
        !wait_until([&] {
            yunlink::AuthorityLease lease{};
            return vehicle.current_authority_for_target(ugv_target, &lease);
        })) {
        std::cerr << "UGV authority failed\n";
        return 7;
    }

    std::atomic<bool> command_seen{false};
    std::atomic<bool> state_seen{false};
    vehicle.command_subscriber().subscribe_ugv_control(
        [&](const yunlink::InboundCommandView<yunlink::UgvControlCommand>& view) {
            command_seen.store(view.inbound.envelope.target.target_type ==
                                   yunlink::AgentType::kUgv &&
                               view.payload.control_cmd == 5);
        });
    ground.state_subscriber().subscribe_ugv_control_state(
        [&](const yunlink::TypedMessage<yunlink::UgvControlStateSnapshot>& message) {
            state_seen.store(message.envelope.source.agent_type == yunlink::AgentType::kUgv &&
                             message.payload.agent_id == 3);
        });
    if (ground.command_publisher().publish_ugv_control(peer_id, session_id, ugv_target, command) !=
            yunlink::ErrorCode::kOk ||
        !wait_until([&] { return command_seen.load(); })) {
        std::cerr << "UGV command runtime dispatch failed\n";
        return 8;
    }
    yunlink::SessionDescriptor session{};
    if (!vehicle.session_server().find_active_session(&session) ||
        vehicle.publish_state_from(
            identity(yunlink::AgentType::kUgv, 3),
            session.peer.id,
            yunlink::TargetSelector::for_entity(yunlink::AgentType::kGroundStation, 30),
            control_state,
            session_id) != yunlink::ErrorCode::kOk ||
        !wait_until([&] { return state_seen.load(); })) {
        std::cerr << "UGV state runtime dispatch failed\n";
        return 9;
    }

    ground.stop();
    vehicle.stop();
    return 0;
}
