/**
 * @file src/runtime/core/lifecycle.cpp
 * @brief Runtime 主分发与生命周期实现。
 */

#include "security.hpp"

namespace yunlink {

Runtime::Runtime()
    : impl_(std::make_unique<Impl>()), session_client_(this), session_server_(this),
      command_publisher_(this), command_subscriber_(this), state_subscriber_(this),
      event_subscriber_(this) {}

Runtime::~Runtime() {
    stop();
}

ErrorCode Runtime::start(const RuntimeConfig& config) {
    if (is_started_) {
        return ErrorCode::kOk;
    }
    config_ = config;

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
    apply_runtime_security_tag(config_, &outbound);
    const int sent_client = tcp_clients_.send_envelope(peer_id, outbound);
    if (sent_client >= 0) {
        return ErrorCode::kOk;
    }
    const int sent_server = tcp_server_.send_envelope(peer_id, outbound);
    return sent_server >= 0 ? ErrorCode::kOk : ErrorCode::kConnectError;
}

ErrorCode Runtime::reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope) {
    if (!is_started_) {
        return ErrorCode::kRuntimeStopped;
    }
    SecureEnvelope outbound = envelope;
    apply_runtime_security_tag(config_, &outbound);
    if (inbound.transport == TransportType::kTcpServer) {
        return tcp_server_.send_envelope(inbound.peer.id, outbound) >= 0 ? ErrorCode::kOk
                                                                         : ErrorCode::kConnectError;
    }
    if (inbound.transport == TransportType::kTcpClient) {
        return tcp_clients_.send_envelope(inbound.peer.id, outbound) >= 0
                   ? ErrorCode::kOk
                   : ErrorCode::kConnectError;
    }
    return udp_.send_envelope_unicast(outbound, inbound.peer.ip, inbound.peer.port) >= 0
               ? ErrorCode::kOk
               : ErrorCode::kConnectError;
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
