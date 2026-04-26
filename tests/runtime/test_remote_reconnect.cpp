#include <chrono>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 150, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

bool reconnect_until_ok(yunlink::Runtime& runtime,
                        const std::string& ip,
                        uint16_t port,
                        std::string* peer_id) {
    return wait_until([&]() {
        return runtime.tcp_clients().connect_peer(ip, port, peer_id) == yunlink::ErrorCode::kOk;
    });
}

}  // namespace

int main() {
    yunlink::Runtime air;
    yunlink::Runtime ground;

    yunlink::RuntimeConfig air_cfg;
    air_cfg.udp_bind_port = 12410;
    air_cfg.udp_target_port = 12410;
    air_cfg.tcp_listen_port = 12510;
    air_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    air_cfg.self_identity.agent_id = 1;
    air_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    air_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig ground_cfg;
    ground_cfg.udp_bind_port = 12411;
    ground_cfg.udp_target_port = 12411;
    ground_cfg.tcp_listen_port = 12511;
    ground_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    ground_cfg.self_identity.agent_id = 7;
    ground_cfg.self_identity.role = yunlink::EndpointRole::kController;
    ground_cfg.shared_secret = "yunlink-secret";

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk ||
        ground.start(ground_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    const auto target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    std::string peer_id;
    if (ground.tcp_clients().connect_peer("127.0.0.1", air_cfg.tcp_listen_port, &peer_id) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "initial connect failed\n";
        return 2;
    }

    const uint64_t session1 =
        ground.session_client().open_active_session(peer_id, "reconnect-ground-1");
    if (session1 == 0 ||
        ground.request_authority(
            peer_id, session1, target, yunlink::ControlSource::kGroundStation, 1500) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "initial session setup failed\n";
        return 3;
    }

    yunlink::AuthorityLease lease{};
    if (!wait_until(
            [&]() { return air.current_authority(&lease) && lease.session_id == session1; })) {
        std::cerr << "initial authority missing\n";
        return 4;
    }

    if (ground.release_authority(peer_id, session1, target) != yunlink::ErrorCode::kOk) {
        std::cerr << "initial release failed\n";
        return 5;
    }
    if (!wait_until([&]() {
            yunlink::AuthorityLease current{};
            return !air.current_authority(&current);
        })) {
        std::cerr << "authority not released before restart\n";
        return 6;
    }

    yunlink::GotoCommand goto_cmd{};
    goto_cmd.x_m = 8.0F;
    goto_cmd.y_m = 1.0F;
    goto_cmd.z_m = 2.0F;
    goto_cmd.yaw_rad = 0.25F;

    air.stop();
    yunlink::SessionDescriptor local_session{};
    if (!wait_until([&]() {
            return ground.session_server().describe_session(peer_id, session1, &local_session) &&
                   local_session.state == yunlink::SessionState::kLost;
        })) {
        std::cerr << "old local session did not become lost after disconnect\n";
        return 7;
    }
    if (ground.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kRejected) {
        std::cerr << "lost session did not reject command publish\n";
        return 8;
    }

    if (air.start(air_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "air restart failed\n";
        return 9;
    }

    if (!reconnect_until_ok(ground, "127.0.0.1", air_cfg.tcp_listen_port, &peer_id)) {
        std::cerr << "reconnect after air restart failed\n";
        return 10;
    }
    if (ground.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kRejected) {
        std::cerr << "reconnected old session unexpectedly became usable\n";
        return 11;
    }

    const uint64_t session2 =
        ground.session_client().open_active_session(peer_id, "reconnect-ground-2");
    if (session2 == 0 || session2 == session1 ||
        ground.request_authority(
            peer_id, session2, target, yunlink::ControlSource::kGroundStation, 1500) !=
            yunlink::ErrorCode::kOk) {
        std::cerr << "recovered session setup failed\n";
        return 12;
    }
    if (ground.command_publisher().publish_goto(peer_id, session1, target, goto_cmd) !=
        yunlink::ErrorCode::kRejected) {
        std::cerr << "old session revived after new session opened\n";
        return 13;
    }
    if (!wait_until(
            [&]() { return air.current_authority(&lease) && lease.session_id == session2; })) {
        std::cerr << "recovered authority missing\n";
        return 14;
    }

    std::vector<yunlink::CommandPhase> phases;
    const size_t result_token = ground.event_subscriber().subscribe_command_results(
        [&](const yunlink::CommandResultView& view) {
            if (view.envelope.session_id == session2) {
                phases.push_back(view.payload.phase);
            }
        });

    if (ground.command_publisher().publish_goto(peer_id, session2, target, goto_cmd) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "recovered publish failed\n";
        return 15;
    }
    if (!wait_until([&]() { return phases.size() >= 4; })) {
        std::cerr << "recovered command flow missing\n";
        return 16;
    }

    ground.event_subscriber().unsubscribe(result_token);
    ground.stop();
    air.stop();
    return 0;
}
