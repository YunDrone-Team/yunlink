#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>

#include "yunlink/runtime/runtime_v2.hpp"
#include "yunlink/core/core_messages_v2.hpp"

int main() {
    using namespace yunlink::v2;
    Runtime server;
    Runtime client;
    RuntimeConfig server_config;
    server_config.endpoint_uid = "endpoint.server";
    server_config.tcp_listen_port = 19696;
    server_config.entities = {{"entity.alpha", "robot", "Alpha"}};
    server_config.profiles = {{"org.yunlink.mobility", 1, 0, "mobility-digest"},
                              {"com.example.server", 1, 0, "server-digest"}};
    RuntimeConfig client_config;
    client_config.endpoint_uid = "endpoint.client";
    client_config.tcp_listen_port = 19697;
    client_config.profiles = {{"org.yunlink.mobility", 1, 2, "mobility-digest"},
                              {"com.example.unknown", 1, 0, "unknown-digest"}};
    assert(server.start(server_config) == ErrorCode::kOk);
    assert(client.start(client_config) == ErrorCode::kOk);

    std::mutex mutex;
    std::condition_variable changed;
    SessionInfo active;
    bool action_received = false;
    client.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kActive) {
            std::lock_guard<std::mutex> lock(mutex);
            active = event.session;
            changed.notify_all();
        }
    });
    server.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kEnvelope &&
            event.envelope.family == MessageFamily::kAction) {
            std::lock_guard<std::mutex> lock(mutex);
            action_received = true;
            changed.notify_all();
        }
    });
    Peer peer;
    assert(client.connect_peer("127.0.0.1", server_config.tcp_listen_port, &peer) ==
           ErrorCode::kOk);
    const uint64_t session_id = client.open_session(peer.id);
    assert(session_id != 0);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(
            lock, std::chrono::seconds(3), [&]() { return active.session_id == session_id; }));
    }
    assert(active.authenticated);
    assert(active.has_profile("org.yunlink.mobility", 1));
    assert(active.negotiated_profiles.at("org.yunlink.mobility").minor == 0);
    assert(active.rejected_profiles.size() == 1);
    assert(active.rejected_profiles.front() == "com.example.server");

    const auto target = TargetSelector::entity("entity.alpha");
    MessageHandle handle;
    assert(client.publish(peer.id,
                          session_id,
                          MessageFamily::kAuthority,
                          static_cast<uint8_t>(AuthorityOperation::kClaim),
                          target,
                          {"yunlink.core", 2, 0, "authority.request"},
                          encode(AuthorityRequest{"org.yunlink.mobility", 2000, false}),
                          &handle) == ErrorCode::kOk);
    assert(client.publish(peer.id,
                          session_id,
                          MessageFamily::kAction,
                          static_cast<uint8_t>(ActionOperation::kGoal),
                          target,
                          {"org.yunlink.mobility", 1, 0, "GotoGoal"},
                          {1, 2, 3},
                          &handle) == ErrorCode::kOk);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(lock, std::chrono::seconds(3), [&]() { return action_received; }));
    }

    client.stop();
    server.stop();
    return 0;
}
