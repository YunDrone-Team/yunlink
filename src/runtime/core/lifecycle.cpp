/**
 * @file src/runtime/core/lifecycle.cpp
 * @brief Runtime 主分发与生命周期实现。
 */

#include "security.hpp"
#include "send_trace.hpp"

namespace yunlink {
namespace {

TransportPreference transport_for_qos(const RuntimeQosPolicy& policy, QosClass qos_class) {
    switch (qos_class) {
    case QosClass::kReliableOrdered:
        return TransportPreference::kTcp;
    case QosClass::kReliableLatest:
        return policy.reliable_latest;
    case QosClass::kBestEffort:
        return policy.best_effort;
    case QosClass::kBulk:
        return policy.bulk;
    }
    return TransportPreference::kTcp;
}

TransportPreference transport_for_envelope(const RuntimeConfig& config, SecureEnvelope* envelope) {
    for (const RuntimeQosChannelPolicy& channel : config.qos_channel_policies) {
        if (channel.message_family == envelope->message_family &&
            channel.message_type == envelope->message_type) {
            envelope->qos_class = channel.qos_class;
            return channel.transport;
        }
    }
    return transport_for_qos(config.qos_policy, envelope->qos_class);
}

}  // namespace

Runtime::Runtime()
    : impl_(std::make_unique<Impl>()), session_client_(this), session_server_(this),
      command_publisher_(this), command_subscriber_(this), state_subscriber_(this),
      event_subscriber_(this), system_service_publisher_(this), system_service_subscriber_(this),
      configuration_service_publisher_(this), configuration_service_subscriber_(this) {}

Runtime::~Runtime() {
    stop();
}

ErrorCode Runtime::start(const RuntimeConfig& config) {
    if (is_started_) {
        return ErrorCode::kOk;
    }
    config_ = config;
    std::atomic_store(
        &impl_->local_managed_identities,
        std::make_shared<const std::vector<EndpointIdentity>>(config.managed_identities));
    bus_.configure_packet_trace(runtime_packet_trace_config(config_));

    const ErrorCode ec_udp = udp_.start(config_, &bus_);
    if (ec_udp != ErrorCode::kOk) {
        return ec_udp;
    }

    const ErrorCode ec_clients = tcp_clients_.start(config_, &bus_);
    if (ec_clients != ErrorCode::kOk) {
        udp_.stop();
        return ec_clients;
    }

    const ErrorCode ec_server = tcp_server_.start(config_, &bus_);
    if (ec_server != ErrorCode::kOk) {
        tcp_clients_.stop();
        udp_.stop();
        return ec_server;
    }

    impl_->bus_token =
        bus_.subscribe_envelope([this](const EnvelopeEvent& ev) { handle_envelope(ev); });
    impl_->link_bus_token =
        bus_.subscribe_link([this](const LinkEvent& ev) { handle_link_event(ev); });
    is_started_ = true;
    return ErrorCode::kOk;
}

ErrorCode Runtime::set_managed_identities(const std::vector<EndpointIdentity>& identities) {
    std::vector<EndpointIdentity> normalized;
    normalized.reserve(identities.size());
    for (const auto& identity : identities) {
        if (identity.agent_type == AgentType::kUnknown || identity.agent_id == 0) {
            return ErrorCode::kInvalidArgument;
        }
        if (runtime_same_entity(config_.self_identity, identity)) {
            continue;
        }
        if (std::any_of(normalized.begin(), normalized.end(), [&](const auto& existing) {
                return runtime_same_entity(existing, identity);
            })) {
            return ErrorCode::kInvalidArgument;
        }
        normalized.push_back(identity);
    }
    const auto snapshot =
        std::make_shared<const std::vector<EndpointIdentity>>(std::move(normalized));
    std::atomic_store(&impl_->local_managed_identities, snapshot);

    std::lock_guard<std::mutex> lock(impl_->mu);
    for (auto it = impl_->authorities.begin(); it != impl_->authorities.end();) {
        if (!runtime_matches_local_target(config_.self_identity, *snapshot, it->second.target)) {
            it = impl_->authorities.erase(it);
        } else {
            ++it;
        }
    }
    return ErrorCode::kOk;
}

std::vector<SessionDescriptor> Runtime::active_sessions() const {
    std::vector<SessionDescriptor> sessions;
    std::lock_guard<std::mutex> lock(impl_->mu);
    sessions.reserve(impl_->sessions.size());
    for (const auto& entry : impl_->sessions) {
        if (entry.second.state == SessionState::kActive) {
            sessions.push_back(entry.second);
        }
    }
    return sessions;
}

bool Runtime::local_source_allowed(const EndpointIdentity& source) const {
    const auto snapshot = std::atomic_load(&impl_->local_managed_identities);
    return runtime_local_source_allowed(config_.self_identity, *snapshot, source);
}

bool Runtime::matches_local_target(const TargetSelector& target) const {
    const auto snapshot = std::atomic_load(&impl_->local_managed_identities);
    return runtime_matches_local_target(config_.self_identity, *snapshot, target);
}

EndpointIdentity Runtime::source_for_target(const TargetSelector& target) const {
    const auto snapshot = std::atomic_load(&impl_->local_managed_identities);
    return runtime_source_for_target(config_.self_identity, *snapshot, target);
}

void Runtime::stop() {
    if (!is_started_) {
        return;
    }
    bus_.unsubscribe(impl_->bus_token);
    bus_.unsubscribe(impl_->link_bus_token);
    tcp_server_.stop();
    tcp_clients_.stop();
    udp_.stop();
    {
        std::lock_guard<std::mutex> lock(impl_->mu);
        impl_->sessions.clear();
        impl_->authorities.clear();
        impl_->active_bulk_channels.clear();
        impl_->reliable_latest_watermarks.clear();
        impl_->command_result_from_status_seen.clear();
        impl_->trajectory_accumulators.clear();
        impl_->security_replay_keys.clear();
    }
    impl_->bus_token = 0;
    impl_->link_bus_token = 0;
    is_started_ = false;
}

uint64_t Runtime::allocate_session_id() {
    std::lock_guard<std::mutex> lock(impl_->mu);
    return impl_->next_session_id++;
}

uint64_t Runtime::allocate_message_id() {
    std::lock_guard<std::mutex> lock(impl_->mu);
    return impl_->next_message_id++;
}

ErrorCode Runtime::send_envelope_to_peer(const std::string& peer_id,
                                         const SecureEnvelope& envelope) {
    if (!is_started_) {
        return ErrorCode::kRuntimeStopped;
    }
    SecureEnvelope outbound = envelope;
    if (outbound.message_family != MessageFamily::kSession &&
        !local_source_allowed(outbound.source)) {
        return ErrorCode::kUnauthorized;
    }
    apply_runtime_security_tag(config_, &outbound);
    PeerInfo trace_peer;
    trace_peer.id = peer_id;

    const auto send_tcp = [&]() {
        trace_encoded_send(bus_, config_, TransportType::kTcpClient, trace_peer, outbound);
        const int sent_client = tcp_clients_.send_envelope(peer_id, outbound);
        if (sent_client >= 0) {
            trace_send_result(
                bus_, config_, TransportType::kTcpClient, trace_peer, outbound, ErrorCode::kOk, "");
            return ErrorCode::kOk;
        }
        trace_send_result(bus_,
                          config_,
                          TransportType::kTcpClient,
                          trace_peer,
                          outbound,
                          ErrorCode::kConnectError,
                          "tcp-client-send-failed");
        trace_encoded_send(bus_, config_, TransportType::kTcpServer, trace_peer, outbound);
        const int sent_server = tcp_server_.send_envelope(peer_id, outbound);
        if (sent_server >= 0) {
            trace_send_result(
                bus_, config_, TransportType::kTcpServer, trace_peer, outbound, ErrorCode::kOk, "");
            return ErrorCode::kOk;
        }
        trace_send_result(bus_,
                          config_,
                          TransportType::kTcpServer,
                          trace_peer,
                          outbound,
                          ErrorCode::kConnectError,
                          "tcp-send-failed");
        return ErrorCode::kConnectError;
    };

    if (transport_for_envelope(config_, &outbound) == TransportPreference::kTcp) {
        return send_tcp();
    }

    SessionDescriptor session{};
    const bool has_session = describe_session_internal(peer_id, outbound.session_id, &session) &&
                             session.state == SessionState::kActive &&
                             !session.udp_peer.ip.empty() && session.udp_peer.port > 0;
    if (has_session) {
        trace_encoded_send(bus_, config_, TransportType::kUdpUnicast, session.udp_peer, outbound);
        if (udp_.send_envelope_unicast(outbound, session.udp_peer.ip, session.udp_peer.port) >= 0) {
            trace_send_result(bus_,
                              config_,
                              TransportType::kUdpUnicast,
                              session.udp_peer,
                              outbound,
                              ErrorCode::kOk,
                              "");
            return ErrorCode::kOk;
        }
        trace_send_result(bus_,
                          config_,
                          TransportType::kUdpUnicast,
                          session.udp_peer,
                          outbound,
                          ErrorCode::kConnectError,
                          "udp-send-failed");
    }

    return config_.qos_policy.udp_fallback_to_tcp ? send_tcp() : ErrorCode::kConnectError;
}

ErrorCode Runtime::reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope) {
    if (!is_started_) {
        return ErrorCode::kRuntimeStopped;
    }
    SecureEnvelope outbound = envelope;
    apply_runtime_security_tag(config_, &outbound);
    if (inbound.transport == TransportType::kTcpServer) {
        trace_encoded_send(bus_, config_, inbound.transport, inbound.peer, outbound);
        const ErrorCode result = tcp_server_.send_envelope(inbound.peer.id, outbound) >= 0
                                     ? ErrorCode::kOk
                                     : ErrorCode::kConnectError;
        trace_send_result(bus_,
                          config_,
                          inbound.transport,
                          inbound.peer,
                          outbound,
                          result,
                          "reply-tcp-server-send-failed");
        return result;
    }
    if (inbound.transport == TransportType::kTcpClient) {
        trace_encoded_send(bus_, config_, inbound.transport, inbound.peer, outbound);
        const ErrorCode result = tcp_clients_.send_envelope(inbound.peer.id, outbound) >= 0
                                     ? ErrorCode::kOk
                                     : ErrorCode::kConnectError;
        trace_send_result(bus_,
                          config_,
                          inbound.transport,
                          inbound.peer,
                          outbound,
                          result,
                          "reply-tcp-client-send-failed");
        return result;
    }
    trace_encoded_send(bus_, config_, TransportType::kUdpUnicast, inbound.peer, outbound);
    const ErrorCode result =
        udp_.send_envelope_unicast(outbound, inbound.peer.ip, inbound.peer.port) >= 0
            ? ErrorCode::kOk
            : ErrorCode::kConnectError;
    trace_send_result(bus_,
                      config_,
                      TransportType::kUdpUnicast,
                      inbound.peer,
                      outbound,
                      result,
                      "reply-udp-send-failed");
    return result;
}

bool Runtime::describe_session_internal(uint64_t session_id, SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    for (const auto& entry : impl_->sessions) {
        if (entry.second.session_id != session_id) {
            continue;
        }
        if (out != nullptr) {
            *out = entry.second;
        }
        return true;
    }
    return false;
}

bool Runtime::describe_session_internal(const std::string& peer_id,
                                        uint64_t session_id,
                                        SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->sessions.find(runtime_session_key(peer_id, session_id));
    if (it == impl_->sessions.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second;
    }
    return true;
}

bool Runtime::find_active_session_internal(SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    for (const auto& entry : impl_->sessions) {
        if (entry.second.state != SessionState::kActive) {
            continue;
        }
        if (out != nullptr) {
            *out = entry.second;
        }
        return true;
    }
    return false;
}

}  // namespace yunlink
