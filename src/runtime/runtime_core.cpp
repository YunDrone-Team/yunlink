/**
 * @file src/runtime/runtime_core.cpp
 * @brief Runtime 主分发与生命周期实现。
 */

#include "runtime_internal.hpp"

namespace sunraycom {

Runtime::Runtime()
    : impl_(std::make_unique<Impl>()), session_client_(this), session_server_(this),
      command_publisher_(this), state_subscriber_(this), event_subscriber_(this) {}

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
    is_started_ = true;
    return ErrorCode::kOk;
}

void Runtime::stop() {
    if (!is_started_) {
        return;
    }
    bus_.unsubscribe(impl_->bus_token);
    tcp_server_.stop();
    tcp_clients_.stop();
    udp_.stop();
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
    const int sent_client = tcp_clients_.send_envelope(peer_id, envelope);
    if (sent_client >= 0) {
        return ErrorCode::kOk;
    }
    const int sent_server = tcp_server_.send_envelope(peer_id, envelope);
    return sent_server >= 0 ? ErrorCode::kOk : ErrorCode::kConnectError;
}

ErrorCode Runtime::reply_on_route(const EnvelopeEvent& inbound, const SecureEnvelope& envelope) {
    if (inbound.transport == TransportType::kTcpServer) {
        return tcp_server_.send_envelope(inbound.peer.id, envelope) >= 0 ? ErrorCode::kOk
                                                                         : ErrorCode::kConnectError;
    }
    if (inbound.transport == TransportType::kTcpClient) {
        return tcp_clients_.send_envelope(inbound.peer.id, envelope) >= 0
                   ? ErrorCode::kOk
                   : ErrorCode::kConnectError;
    }
    return udp_.send_envelope_unicast(envelope, inbound.peer.ip, inbound.peer.port) >= 0
               ? ErrorCode::kOk
               : ErrorCode::kConnectError;
}

bool Runtime::describe_session_internal(uint64_t session_id, SessionDescriptor* out) const {
    std::lock_guard<std::mutex> lock(impl_->mu);
    const auto it = impl_->sessions.find(session_id);
    if (it == impl_->sessions.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second;
    }
    return true;
}

void Runtime::handle_envelope(const EnvelopeEvent& ev) {
    switch (ev.envelope.message_family) {
    case MessageFamily::kSession:
        handle_session_envelope(ev);
        return;
    case MessageFamily::kAuthority:
        handle_authority_envelope(ev);
        return;
    case MessageFamily::kCommand:
        handle_command_envelope(ev);
        return;
    case MessageFamily::kStateSnapshot:
        handle_state_snapshot_envelope(ev);
        return;
    case MessageFamily::kStateEvent:
        handle_state_event_envelope(ev);
        return;
    case MessageFamily::kCommandResult:
        handle_command_result_envelope(ev);
        return;
    case MessageFamily::kBulkChannelDescriptor:
        return;
    }
}

}  // namespace sunraycom
