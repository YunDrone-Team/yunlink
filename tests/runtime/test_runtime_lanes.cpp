#include <cassert>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

#include "yunlink/runtime/runtime_v2.hpp"

int main() {
    using namespace yunlink::v2;
    Runtime server;
    Runtime client;
    RuntimeConfig server_config;
    server_config.endpoint_uid = "endpoint.server";
    server_config.tcp_listen_port = 0;
    server_config.profiles = {{"org.yunlink.visual", 1, 0, "visual-sample-v1"},
                              {"org.yunlink.mobility", 1, 0, "mobility-v1"}};
    RuntimeConfig client_config;
    client_config.endpoint_uid = "endpoint.client";
    client_config.tcp_listen_port = 0;
    client_config.profiles = server_config.profiles;
    assert(server.start(server_config) == ErrorCode::kOk);
    server_config.tcp_listen_port = server.listening_port();
    assert(client.start(client_config) == ErrorCode::kOk);

    std::mutex mutex;
    std::condition_variable changed;
    SessionInfo active;
    bool visual_received = false;
    bool control_received = false;
    bool session_lost = false;
    bool server_session_lost = false;
    client.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kActive) {
            std::lock_guard<std::mutex> lock(mutex);
            active = event.session;
            changed.notify_all();
        }
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kLost) {
            std::lock_guard<std::mutex> lock(mutex);
            session_lost = true;
            changed.notify_all();
        }
    });
    server.subscribe([&](const RuntimeEvent& event) {
        if (event.kind == RuntimeEventKind::kSession &&
            event.session.state == SessionState::kLost) {
            std::lock_guard<std::mutex> lock(mutex);
            server_session_lost = true;
            changed.notify_all();
        }
        if (event.kind == RuntimeEventKind::kEnvelope &&
            event.envelope.family == MessageFamily::kStream &&
            event.envelope.qos_class == QosClass::kBestEffort) {
            std::lock_guard<std::mutex> lock(mutex);
            visual_received = true;
            changed.notify_all();
        }
        if (event.kind == RuntimeEventKind::kEnvelope &&
            event.envelope.family == MessageFamily::kStream &&
            event.envelope.qos_class == QosClass::kReliableOrdered) {
            std::lock_guard<std::mutex> lock(mutex);
            control_received = true;
            changed.notify_all();
        }
    });

    Peer peer;
    assert(client.connect_peer("127.0.0.1", server.listening_port(), &peer) == ErrorCode::kOk);
    const uint64_t session_id = client.open_session(peer.id);
    assert(session_id != 0);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(
            lock, std::chrono::seconds(3), [&]() { return active.session_id == session_id; }));
    }
    assert(client.bind_lossy_lane(peer.id, session_id) == ErrorCode::kOk);
    assert(client.session_has_lossy_lane(peer.id, session_id));
    assert(!client.session_has_lossy_lane(peer.id, session_id + 1));
    SessionInfo bound;
    assert(client.session(peer.id, session_id, &bound));
    assert(!bound.lossy_peer_id.empty());

    MessageHandle handle;
    assert(client.publish(peer.id,
                          session_id,
                          MessageFamily::kStream,
                          4,
                          TargetSelector::broadcast(),
                          {"org.yunlink.visual", 1, 0, "Sample"},
                          {1, 2, 3, 4},
                          &handle,
                          0,
                          1000,
                          QosClass::kBestEffort) == ErrorCode::kOk);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(lock, std::chrono::seconds(3), [&]() { return visual_received; }));
    }

    assert(client.session(peer.id, session_id, &bound));
    client.close_peer(bound.lossy_peer_id);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    assert(!client.session_has_lossy_lane(peer.id, session_id));
    {
        std::lock_guard<std::mutex> lock(mutex);
        assert(!session_lost);
    }
    assert(client.session(peer.id, session_id, &bound));
    assert(bound.state == SessionState::kActive);
    assert(client.bind_lossy_lane(peer.id, session_id) == ErrorCode::kOk);
    assert(client.session_has_lossy_lane(peer.id, session_id));

    assert(client.publish(peer.id,
                          session_id,
                          MessageFamily::kStream,
                          4,
                          TargetSelector::broadcast(),
                          {"yunlink.core", 2, 0, "stream.sample"},
                          {9},
                          &handle,
                          0,
                          1000,
                          QosClass::kReliableOrdered) == ErrorCode::kOk);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(changed.wait_for(lock, std::chrono::seconds(3), [&]() { return control_received; }));
    }

    client.close_peer(peer.id);
    {
        std::unique_lock<std::mutex> lock(mutex);
        assert(
            changed.wait_for(lock, std::chrono::seconds(3), [&]() { return server_session_lost; }));
    }
    assert(server.listening_port() != 0);
    Runtime replacement;
    assert(replacement.start(client_config) == ErrorCode::kOk);
    Peer replacement_peer;
    assert(replacement.connect_peer("127.0.0.1", server.listening_port(), &replacement_peer) ==
           ErrorCode::kOk);
    replacement.stop();
    client.stop();
    server.stop();
    return 0;
}
