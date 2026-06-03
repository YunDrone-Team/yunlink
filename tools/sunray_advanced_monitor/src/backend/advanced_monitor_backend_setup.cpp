#include "backend/advanced_monitor_backend.hpp"

#include <algorithm>
#include <sstream>

namespace {

std::string endpoint_text(const std::string& ip, uint16_t port) {
    std::ostringstream oss;
    oss << ip << ":" << port;
    return oss.str();
}

}  // namespace

void AdvancedMonitorBackend::load_params() {
    pnh_.param<std::string>("remote_ip", remote_ip_, "127.0.0.1");
    pnh_.param<int>("remote_tcp_port", remote_tcp_port_, 9696);
    pnh_.param<int>("udp_bind_port", udp_bind_port_, 9797);
    pnh_.param<int>("udp_target_port", udp_target_port_, 9898);
    pnh_.param<int>("tcp_listen_port", tcp_listen_port_, 9797);
    pnh_.param<int>("agent_id", agent_id_, 1);
    pnh_.param<std::string>("agent_name", agent_name_, "uav");
    pnh_.param<std::string>("shared_secret", shared_secret_, "yunlink-default-secret");
    pnh_.param<std::string>("node_name", node_name_, "sunray_advanced_monitor");
    pnh_.param<int>("log_limit", log_limit_raw_, 500);
    pnh_.param<double>("state_timeout_sec", state_timeout_sec_, 2.0);

    int authority_ttl_ms_raw = static_cast<int>(authority_ttl_ms_);
    pnh_.param<int>("authority_ttl_ms", authority_ttl_ms_raw, 3000);
    log_limit_ = static_cast<size_t>(std::max(100, log_limit_raw_));
    state_timeout_sec_ = std::max(0.1, state_timeout_sec_);
    authority_ttl_ms_ = static_cast<uint32_t>(std::max(1000, authority_ttl_ms_raw));

    int command_history_limit_raw = static_cast<int>(command_history_limit_);
    pnh_.param<int>("command_history_limit", command_history_limit_raw, 32);
    command_history_limit_ = static_cast<size_t>(std::max(8, command_history_limit_raw));

    int command_timeout_ms_raw = static_cast<int>(command_timeout_ms_);
    pnh_.param<int>("command_timeout_ms", command_timeout_ms_raw, 3000);
    command_timeout_ms_ = static_cast<uint64_t>(std::max(1000, command_timeout_ms_raw));

    const std::string agent_key = "/" + agent_name_ + std::to_string(agent_id_);
    topics_["local_odom"].ros_topic = agent_key + "/sunray/localization/local_odom";
    topics_["odom_state"].ros_topic = agent_key + "/sunray/localization/odom_state";
    topics_["uav_control_state"].ros_topic = agent_key + "/sunray/uav_control/control_state";
    topics_["mavros_state"].ros_topic = agent_key + "/mavros/state";
    topics_["px4_state"].ros_topic = agent_key + "/sunray/px4_state";
}

void AdvancedMonitorBackend::start_runtime() {
    yunlink::RuntimeConfig cfg;
    cfg.udp_bind_port = clamp_port(udp_bind_port_);
    cfg.udp_target_port = clamp_port(udp_target_port_);
    cfg.tcp_listen_port = clamp_port(tcp_listen_port_);
    cfg.shared_secret = shared_secret_;
    cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    cfg.self_identity.agent_id = static_cast<uint32_t>(1000 + std::max(agent_id_, 0));
    cfg.self_identity.role = yunlink::EndpointRole::kController;

    const auto ec = runtime_.start(cfg);
    if (ec != yunlink::ErrorCode::kOk) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.runtime_started = false;
            connection_.runtime_status = "FAILED";
            connection_.session_state = "RUNTIME_FAILED";
            connection_.last_error =
                "Runtime 启动失败，ec=" + std::to_string(static_cast<int>(ec));
            connection_.updated_at_ms = wall_time_ms();
        }
        log(MonitorLogLevel::kError,
            MonitorLogSource::kRuntime,
            "Runtime 启动失败，ec=" + std::to_string(static_cast<int>(ec)));
        return;
    }

    {
        std::lock_guard<std::mutex> lock(mu_);
        runtime_started_ = true;
        connection_.runtime_started = true;
        connection_.runtime_status = "RUNNING";
        connection_.session_state = "WAITING_CONNECT";
        connection_.last_note = "Runtime 已启动，等待会话";
        connection_.updated_at_ms = wall_time_ms();
    }
    log(MonitorLogLevel::kInfo, MonitorLogSource::kRuntime, "Runtime 已启动");
}

void AdvancedMonitorBackend::bind_runtime_diagnostics() {
    link_token_ = runtime_.event_bus().subscribe_link([this](const yunlink::LinkEvent& ev) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.link_up = ev.is_up;
            connection_.link_state = ev.is_up ? "UP" : "DOWN";
            connection_.last_note =
                std::string("链路") + (ev.is_up ? "已建立" : "已断开") + "，peer=" + ev.peer.id;
            connection_.updated_at_ms = wall_time_ms();
        }
        log(ev.is_up ? MonitorLogLevel::kInfo : MonitorLogLevel::kWarn,
            MonitorLogSource::kSession,
            "link " + std::string(ev.is_up ? "UP" : "DOWN") +
                " transport=" + transport_label(ev.transport) + " peer=" + ev.peer.id +
                " (" + endpoint_text(ev.peer.ip, ev.peer.port) + ")");
    });

    error_token_ = runtime_.event_bus().subscribe_error([this](const yunlink::ErrorEvent& ev) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            connection_.last_error = "error code=" + std::to_string(static_cast<int>(ev.code)) +
                                     " transport=" + transport_label(ev.transport) +
                                     " peer=" + ev.peer.id + " msg=" + ev.message;
            connection_.updated_at_ms = wall_time_ms();
        }
        log(MonitorLogLevel::kError,
            MonitorLogSource::kRuntime,
            "error code=" + std::to_string(static_cast<int>(ev.code)) +
                " transport=" + transport_label(ev.transport) + " peer=" + ev.peer.id +
                " (" + endpoint_text(ev.peer.ip, ev.peer.port) + ") msg=" + ev.message);
    });
}

void AdvancedMonitorBackend::setup_reconnect_timer() {
    if (!runtime_started_) {
        return;
    }
    reconnect_timer_ =
        nh_.createTimer(ros::Duration(1.0), &AdvancedMonitorBackend::on_reconnect_timer, this);
}

void AdvancedMonitorBackend::update_config_snapshot() {
    std::lock_guard<std::mutex> lock(mu_);
    connection_.remote_endpoint = endpoint_text(remote_ip_, clamp_port(remote_tcp_port_));
    connection_.listen_endpoint = endpoint_text("0.0.0.0", clamp_port(tcp_listen_port_));
    connection_.udp_bind_endpoint = endpoint_text("0.0.0.0", clamp_port(udp_bind_port_));
    connection_.udp_target_endpoint = endpoint_text(remote_ip_, clamp_port(udp_target_port_));
    connection_.agent_label = "ground/" + agent_name_ + std::to_string(agent_id_);
    connection_.node_name = node_name_;
    connection_.runtime_status = "INIT";
    connection_.session_state = "INIT";
    connection_.link_state = "WAIT";
    connection_.secret_configured = !shared_secret_.empty();
    connection_.last_note = "等待初始化";
    connection_.updated_at_ms = wall_time_ms();
}

void AdvancedMonitorBackend::request_command_authority_if_needed() {
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        return;
    }

    const auto target = command_target();
    bool should_request = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        const uint64_t now_ms = wall_time_ms();
        if (!authority_active_unlocked() &&
            (!authority_pending_ || authority_request_at_ms_ == 0 ||
             now_ms > authority_request_at_ms_ + 1000)) {
            mark_authority_pending_unlocked(now_ms);
            should_request = true;
        }
    }

    if (!should_request) {
        return;
    }

    const auto ec = runtime_.request_authority(peer_id,
                                               session_id,
                                               target,
                                               yunlink::ControlSource::kGroundStation,
                                               authority_ttl_ms_,
                                               false);
    if (ec != yunlink::ErrorCode::kOk) {
        {
            std::lock_guard<std::mutex> lock(mu_);
            authority_pending_ = false;
        }
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kSession,
            "控制权申请发送失败，target=uav/" + std::to_string(std::max(agent_id_, 0)) +
                " ec=" + error_code_label(ec));
        return;
    }
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kSession,
        "已发送控制权申请，target=uav/" + std::to_string(std::max(agent_id_, 0)) +
            " ttl_ms=" + std::to_string(authority_ttl_ms_));
}

yunlink::TargetSelector AdvancedMonitorBackend::command_target() const {
    return yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav,
                                               static_cast<uint32_t>(std::max(agent_id_, 0)));
}

bool AdvancedMonitorBackend::snapshot_send_context(std::string* peer_id, uint64_t* session_id) const {
    std::lock_guard<std::mutex> lock(mu_);
    if (!runtime_started_ || !peer_ready_ || peer_id_.empty() || session_id_ == 0) {
        return false;
    }
    if (peer_id != nullptr) {
        *peer_id = peer_id_;
    }
    if (session_id != nullptr) {
        *session_id = session_id_;
    }
    return true;
}

bool AdvancedMonitorBackend::authority_active_unlocked() const {
    return authority_expires_at_ms_ > wall_time_ms();
}

void AdvancedMonitorBackend::mark_authority_pending_unlocked(uint64_t now_ms) {
    authority_pending_ = true;
    authority_request_at_ms_ = now_ms;
}
