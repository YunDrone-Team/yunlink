#include "backend/advanced_monitor_backend.hpp"

namespace {

std::string session_state_text(yunlink::SessionState state) {
    switch (state) {
    case yunlink::SessionState::kDiscovered:
        return "DISCOVERED";
    case yunlink::SessionState::kHandshaking:
        return "HANDSHAKING";
    case yunlink::SessionState::kAuthenticated:
        return "AUTHENTICATED";
    case yunlink::SessionState::kNegotiated:
        return "NEGOTIATED";
    case yunlink::SessionState::kActive:
        return "ACTIVE";
    case yunlink::SessionState::kDraining:
        return "DRAINING";
    case yunlink::SessionState::kClosed:
        return "CLOSED";
    case yunlink::SessionState::kLost:
        return "LOST";
    case yunlink::SessionState::kInvalid:
        return "INVALID";
    }
    return "UNKNOWN";
}

bool has_remote_target(const std::string& ip, int port) {
    return !ip.empty() && port > 0;
}

}  // namespace

void AdvancedMonitorBackend::poll_runtime() {
    poll_discovery();
    if (!runtime_started_) {
        return;
    }

    refresh_command_timeouts(wall_time_ms());
    refresh_system_service_timeouts(wall_time_ms());

    if (peer_ready_) {
        yunlink::SessionDescriptor desc{};
        if (!runtime_.session_server().describe_session(peer_id_, session_id_, &desc) ||
            desc.state != yunlink::SessionState::kActive) {
            {
                std::lock_guard<std::mutex> lock(mu_);
                peer_ready_ = false;
                peer_id_.clear();
                session_id_ = 0;
                authority_pending_ = false;
                authority_request_at_ms_ = 0;
                authority_expires_at_ms_ = 0;
                connection_.peer_ready = false;
                connection_.peer_id.clear();
                connection_.session_id = 0;
                connection_.session_state = "RECONNECTING";
                connection_.last_note = "会话已断开，准备重连";
                connection_.updated_at_ms = wall_time_ms();
            }
            log(MonitorLogLevel::kWarn, MonitorLogSource::kConnection, "会话已断开，准备重连");
            return;
        }

        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.peer_ready = true;
            connection_.peer_id = desc.peer.id;
            connection_.session_id = desc.session_id;
            connection_.session_state = session_state_text(desc.state);
            connection_.last_note = "会话保持活跃";
            connection_.updated_at_ms = wall_time_ms();
        }
        request_command_authority_if_needed();
        return;
    }

    const bool has_selected_discovery = !selected_discovery_device_key().empty();
    const bool has_direct_target = has_remote_target(remote_ip_, remote_tcp_port_);
    if (!has_selected_discovery && !has_direct_target) {
        std::lock_guard<std::mutex> lock(mu_);
        connection_.peer_ready = false;
        connection_.session_state = "WAITING_SELECTION";
        connection_.last_note = "等待发现设备并手动连接";
        connection_.last_error.clear();
        connection_.updated_at_ms = wall_time_ms();
        return;
    }

    if (has_direct_target && !has_selected_discovery) {
        yunlink::SessionDescriptor active_session{};
        if (runtime_.session_server().find_active_session(&active_session)) {
            {
                std::lock_guard<std::mutex> lock(mu_);
                peer_ready_ = true;
                peer_id_ = active_session.peer.id;
                session_id_ = active_session.session_id;
                connection_.peer_ready = true;
                connection_.peer_id = active_session.peer.id;
                connection_.session_id = active_session.session_id;
                connection_.session_state = session_state_text(active_session.state);
                connection_.last_note = "复用现有会话";
                connection_.updated_at_ms = wall_time_ms();
            }
            log(MonitorLogLevel::kInfo,
                MonitorLogSource::kConnection,
                "复用现有会话，对端 peer_id=" + active_session.peer.id +
                    "，session_id=" + std::to_string(active_session.session_id));
            request_command_authority_if_needed();
            return;
        }
    }

    std::string peer_id;
    const auto ec =
        runtime_.tcp_clients().connect_peer(remote_ip_, clamp_port(remote_tcp_port_), &peer_id);
    if (ec != yunlink::ErrorCode::kOk) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.session_state = "CONNECT_RETRYING";
            connection_.last_note = "等待 TCP 连接建立";
            connection_.last_error = "connect_peer failed, ec=" + std::to_string(static_cast<int>(ec));
            connection_.updated_at_ms = wall_time_ms();
        }
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kConnection,
            "连接 YunLink 对端失败，ip=" + remote_ip_ +
                " port=" + std::to_string(clamp_port(remote_tcp_port_)) +
                " ec=" + std::to_string(static_cast<int>(ec)));
        return;
    }

    const uint64_t session_id = runtime_.session_client().open_active_session(peer_id, node_name_);
    if (session_id == 0) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.session_state = "SESSION_OPEN_FAILED";
            connection_.last_note = "TCP 已连接，但会话打开失败";
            connection_.last_error = "open_active_session failed";
            connection_.updated_at_ms = wall_time_ms();
        }
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kConnection,
            "打开会话失败，peer_id=" + peer_id + " node=" + node_name_);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        peer_ready_ = true;
        peer_id_ = peer_id;
        session_id_ = session_id;
        connection_.peer_ready = true;
        connection_.peer_id = peer_id;
        connection_.session_id = session_id;
        connection_.session_state = "ACTIVE";
        connection_.last_note = "已打开新会话";
        connection_.last_error.clear();
        connection_.updated_at_ms = wall_time_ms();
    }
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kConnection,
        "已连接 YunLink 对端，peer_id=" + peer_id + "，session_id=" +
            std::to_string(session_id));
    request_command_authority_if_needed();
}

yunlink::TargetSelector AdvancedMonitorBackend::system_service_target() const {
    return yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav,
                                               static_cast<uint32_t>(std::max(agent_id_, 0)));
}
