/**
 * @file tests/security/test_session_security.cpp
 * @brief shared secret / protocol mismatch security regressions.
 */

#include <chrono>
#include <functional>
#include <iostream>
#include <thread>

#include "yunlink/core/semantic_messages.hpp"
#include "yunlink/runtime/runtime.hpp"

namespace {

bool wait_until(const std::function<bool()>& pred, int retries = 120, int sleep_ms = 20) {
    for (int i = 0; i < retries; ++i) {
        if (pred()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
    }
    return false;
}

template <typename T>
yunlink::SecureEnvelope make_session_envelope(const yunlink::EndpointIdentity& source,
                                              uint64_t session_id,
                                              uint64_t correlation_id,
                                              const T& payload,
                                              uint32_t ttl_ms = 1000) {
    return yunlink::make_typed_envelope(source,
                                        yunlink::TargetSelector::broadcast(yunlink::AgentType::kUnknown),
                                        session_id,
                                        correlation_id,
                                        yunlink::QosClass::kReliableOrdered,
                                        payload,
                                        ttl_ms);
}

}  // namespace

int main() {
    yunlink::Runtime server;
    yunlink::Runtime wrong_secret_client;
    yunlink::Runtime manual_client;

    yunlink::RuntimeConfig server_cfg;
    server_cfg.udp_bind_port = 12360;
    server_cfg.udp_target_port = 12360;
    server_cfg.tcp_listen_port = 12460;
    server_cfg.self_identity.agent_type = yunlink::AgentType::kUav;
    server_cfg.self_identity.agent_id = 1;
    server_cfg.self_identity.role = yunlink::EndpointRole::kVehicle;
    server_cfg.shared_secret = "yunlink-secret";

    yunlink::RuntimeConfig wrong_secret_cfg;
    wrong_secret_cfg.udp_bind_port = 12361;
    wrong_secret_cfg.udp_target_port = 12361;
    wrong_secret_cfg.tcp_listen_port = 12461;
    wrong_secret_cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    wrong_secret_cfg.self_identity.agent_id = 7;
    wrong_secret_cfg.self_identity.role = yunlink::EndpointRole::kController;
    wrong_secret_cfg.shared_secret = "wrong-secret";

    yunlink::RuntimeConfig manual_cfg = wrong_secret_cfg;
    manual_cfg.udp_bind_port = 12362;
    manual_cfg.udp_target_port = 12362;
    manual_cfg.tcp_listen_port = 12462;
    manual_cfg.self_identity.agent_id = 8;
    manual_cfg.shared_secret = "yunlink-secret";

    if (server.start(server_cfg) != yunlink::ErrorCode::kOk ||
        wrong_secret_client.start(wrong_secret_cfg) != yunlink::ErrorCode::kOk ||
        manual_client.start(manual_cfg) != yunlink::ErrorCode::kOk) {
        std::cerr << "runtime start failed\n";
        return 1;
    }

    std::string wrong_secret_peer;
    if (wrong_secret_client.tcp_clients().connect_peer(
            "127.0.0.1", server_cfg.tcp_listen_port, &wrong_secret_peer) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "wrong secret connect failed\n";
        return 2;
    }

    const uint64_t wrong_secret_session =
        wrong_secret_client.session_client().open_active_session(wrong_secret_peer, "bad-secret");
    if (wrong_secret_session == 0) {
        std::cerr << "wrong secret session open failed\n";
        return 3;
    }

    yunlink::SessionDescriptor desc{};
    if (!wait_until([&]() {
            return server.session_server().describe_session(wrong_secret_session, &desc) &&
                   desc.state == yunlink::SessionState::kInvalid;
        })) {
        std::cerr << "shared secret mismatch was not rejected as invalid\n";
        return 4;
    }

    std::string manual_peer;
    if (manual_client.tcp_clients().connect_peer("127.0.0.1", server_cfg.tcp_listen_port, &manual_peer) !=
        yunlink::ErrorCode::kOk) {
        std::cerr << "manual client connect failed\n";
        return 5;
    }

    const uint64_t protocol_session = 9001;
    const uint64_t correlation_id = 8001;
    const auto source = manual_cfg.self_identity;

    yunlink::SessionHello hello{};
    hello.node_name = "protocol-mismatch";
    hello.capability_flags = 0;
    auto hello_env = make_session_envelope(source, protocol_session, correlation_id, hello);
    hello_env.message_id = 50001;

    yunlink::SessionAuthenticate auth{};
    auth.shared_secret = manual_cfg.shared_secret;
    auto auth_env = make_session_envelope(source, protocol_session, correlation_id, auth);
    auth_env.message_id = 50002;

    yunlink::SessionCapabilities caps{};
    caps.capability_flags = 0;
    auto caps_env = make_session_envelope(source, protocol_session, correlation_id, caps);
    caps_env.message_id = 50003;

    yunlink::SessionReady ready{};
    ready.accepted_protocol_major = 2;
    auto ready_env = make_session_envelope(source, protocol_session, correlation_id, ready);
    ready_env.message_id = 50004;
    ready_env.protocol_major = 1;

    if (manual_client.tcp_clients().send_envelope(manual_peer, hello_env) < 0 ||
        manual_client.tcp_clients().send_envelope(manual_peer, auth_env) < 0 ||
        manual_client.tcp_clients().send_envelope(manual_peer, caps_env) < 0 ||
        manual_client.tcp_clients().send_envelope(manual_peer, ready_env) < 0) {
        std::cerr << "manual session handshake send failed\n";
        return 6;
    }

    if (!wait_until([&]() {
            return server.session_server().describe_session(protocol_session, &desc) &&
                   desc.state == yunlink::SessionState::kInvalid;
        })) {
        std::cerr << "protocol mismatch did not prevent active session\n";
        return 7;
    }
    if (server.session_server().has_active_session(protocol_session)) {
        std::cerr << "protocol mismatch session became active unexpectedly\n";
        return 8;
    }

    manual_client.stop();
    wrong_secret_client.stop();
    server.stop();
    return 0;
}
