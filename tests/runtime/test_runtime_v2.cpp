#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "yunlink/runtime/runtime_v2.hpp"
#include "yunlink/core/core_messages_v2.hpp"

int main() {
    using namespace yunlink::v2;
    Runtime server;
    Runtime client;
    RuntimeConfig server_config;
    server_config.endpoint_uid = "endpoint.server";
    server_config.tcp_listen_port = 0;
    server_config.profiles = {{"org.yunlink.mobility", 1, 0, "mobility-digest"},
                              {"com.example.server", 1, 0, "server-digest"},
                              {"com.yundrone.sunray", 2, 0, "sunray-v2"}};
    RuntimeConfig client_config;
    client_config.endpoint_uid = "endpoint.client";
    client_config.tcp_listen_port = 0;
    client_config.profiles = {{"org.yunlink.mobility", 1, 2, "mobility-digest"},
                              {"com.example.unknown", 1, 0, "unknown-digest"},
                              {"com.yundrone.sunray", 1, 0, "sunray-v1"}};
    assert(server.start(server_config) == ErrorCode::kOk);
    server_config.tcp_listen_port = server.listening_port();
    assert(client.start(client_config) == ErrorCode::kOk);
    assert(server.listening_port() != 0);

    std::mutex mutex;
    std::condition_variable changed;
    SessionInfo active;
    SessionInfo server_active;
    bool action_received = false;
    bool link_down = false;
    std::string server_peer_id;
    client.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kLink && !event.link_up) {
            std::lock_guard<std::mutex> lock(mutex);
            link_down = true;
            changed.notify_all();
        }
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kActive) {
            std::lock_guard<std::mutex> lock(mutex);
            active = event.session;
            changed.notify_all();
        }
    });
    server.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kActive) {
            std::lock_guard<std::mutex> lock(mutex);
            server_peer_id = event.peer.id;
            server_active = event.session;
            changed.notify_all();
        }
        if (event.kind == RuntimeEventKind::kEnvelope &&
            event.envelope.family == MessageFamily::kAction) {
            std::lock_guard<std::mutex> lock(mutex);
            action_received = true;
            changed.notify_all();
        }
    });
    Peer peer;
    assert(client.connect_peer("127.0.0.1", server.listening_port(), &peer) == ErrorCode::kOk);
    const uint64_t session_id = client.open_session(peer.id);
    static_cast<void>(session_id);
    assert(session_id != 0);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(
            lock, std::chrono::seconds(3), [&]() { return active.session_id == session_id; }));
    }
    assert(active.authenticated);
    assert(active.has_profile("org.yunlink.mobility", 1));
    assert(active.negotiated_profiles.at("org.yunlink.mobility").minor == 0);
    assert(active.rejected_profiles.size() == 2);
    assert(std::find(active.rejected_profiles.begin(),
                     active.rejected_profiles.end(),
                     "com.example.server") != active.rejected_profiles.end());
    assert(std::find(active.rejected_profiles.begin(),
                     active.rejected_profiles.end(),
                     "com.yundrone.sunray") != active.rejected_profiles.end());
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(
            lock, std::chrono::seconds(3), [&]() { return !server_peer_id.empty(); }));
    }
    assert(std::find(server_active.rejected_profiles.begin(),
                     server_active.rejected_profiles.end(),
                     "com.example.unknown") != server_active.rejected_profiles.end());
    assert(std::find(server_active.rejected_profiles.begin(),
                     server_active.rejected_profiles.end(),
                     "com.yundrone.sunray") != server_active.rejected_profiles.end());

    assert(server.set_entities({{"entity.alpha", "robot", "Alpha"}}) == ErrorCode::kOk);
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
    for (int attempt = 0;
         attempt < 100 &&
         !server.has_authority(server_peer_id, session_id, "entity.alpha", "org.yunlink.mobility");
         ++attempt) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(
        server.has_authority(server_peer_id, session_id, "entity.alpha", "org.yunlink.mobility"));
    assert(
        !server.has_authority("another-peer", session_id, "entity.alpha", "org.yunlink.mobility"));

    assert(client.publish(peer.id,
                          session_id,
                          MessageFamily::kEntityDirectory,
                          static_cast<uint8_t>(DirectoryOperation::kDetachRequest),
                          TargetSelector::endpoint(server_config.endpoint_uid),
                          {"yunlink.core", 2, 0, "attachment_request"},
                          encode(AttachmentRequest{"stale-revision", {"entity.alpha"}}),
                          &handle) == ErrorCode::kOk);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(
        server.has_authority(server_peer_id, session_id, "entity.alpha", "org.yunlink.mobility"));
    server.revoke_authority(server_peer_id, session_id, {"entity.alpha"});
    assert(
        !server.has_authority(server_peer_id, session_id, "entity.alpha", "org.yunlink.mobility"));

    server.stop();
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(lock, std::chrono::seconds(3), [&]() { return link_down; }));
        active = {};
    }
    assert(server.start(server_config) == ErrorCode::kOk);
    Peer reconnected_peer;
    assert(client.connect_peer("127.0.0.1", server.listening_port(), &reconnected_peer) ==
           ErrorCode::kOk);
    assert(reconnected_peer.id == peer.id);
    const uint64_t reconnected_session_id = client.open_session(reconnected_peer.id);
    static_cast<void>(reconnected_session_id);
    assert(reconnected_session_id != 0);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(lock, std::chrono::seconds(3), [&]() {
            return active.session_id == reconnected_session_id;
        }));
    }

    client.stop();

    Runtime fresh_client;
    assert(fresh_client.start(client_config) == ErrorCode::kOk);
    SessionInfo fresh_active;
    std::mutex fresh_mutex;
    std::condition_variable fresh_changed;
    fresh_client.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kActive) {
            std::lock_guard<std::mutex> lock(fresh_mutex);
            fresh_active = event.session;
            fresh_changed.notify_all();
        }
    });
    Peer fresh_peer;
    assert(fresh_client.connect_peer("127.0.0.1", server_config.tcp_listen_port, &fresh_peer) ==
           ErrorCode::kOk);
    const uint64_t fresh_session_id = fresh_client.open_session(fresh_peer.id);
    static_cast<void>(fresh_session_id);
    assert(fresh_session_id != 0);
    {
        std::unique_lock<std::mutex> lock(fresh_mutex);
        assert(fresh_changed.wait_for(lock, std::chrono::seconds(3), [&]() {
            return fresh_active.session_id == fresh_session_id;
        }));
    }
    fresh_client.stop();
    server.stop();
    return 0;
}
