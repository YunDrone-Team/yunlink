#include "backend/advanced_monitor_backend.hpp"

AdvancedMonitorBackend::AdvancedMonitorBackend()
    : nh_(), pnh_("~"), topics_(make_default_monitor_topics()) {
    load_params();
    update_config_snapshot();
    start_runtime();
    bind_runtime_diagnostics();
    bind_yunlink_subscribers();
    bind_ros_subscribers();
    bind_command_feedback();
    setup_reconnect_timer();
}

AdvancedMonitorBackend::~AdvancedMonitorBackend() {
    for (size_t token : state_sub_tokens_) {
        runtime_.state_subscriber().unsubscribe(token);
    }
    if (command_result_token_ != 0) {
        runtime_.event_subscriber().unsubscribe(command_result_token_);
    }
    if (authority_status_token_ != 0) {
        runtime_.event_subscriber().unsubscribe(authority_status_token_);
    }
    if (link_token_ != 0) {
        runtime_.event_bus().unsubscribe(link_token_);
    }
    if (error_token_ != 0) {
        runtime_.event_bus().unsubscribe(error_token_);
    }
    runtime_.stop();
}

MonitorConnectionSnapshot AdvancedMonitorBackend::snapshot_connection() const {
    std::lock_guard<std::mutex> lock(mu_);
    return connection_;
}

std::unordered_map<std::string, MonitorTopicState> AdvancedMonitorBackend::snapshot_topics() const {
    std::lock_guard<std::mutex> lock(mu_);
    return topics_;
}

std::vector<MonitorLogEntry> AdvancedMonitorBackend::snapshot_logs() const {
    std::lock_guard<std::mutex> lock(mu_);
    return logs_;
}

std::vector<MonitorCommandHistoryEntry> AdvancedMonitorBackend::snapshot_command_history() const {
    std::lock_guard<std::mutex> lock(mu_);
    return command_history_;
}

double AdvancedMonitorBackend::state_timeout_sec() const {
    return state_timeout_sec_;
}

bool AdvancedMonitorBackend::can_send_commands() const {
    std::lock_guard<std::mutex> lock(mu_);
    return runtime_started_ && peer_ready_ && session_id_ != 0 && authority_active_unlocked();
}

void AdvancedMonitorBackend::clear_logs() {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.clear();
    throttled_logs_.clear();
}

void AdvancedMonitorBackend::request_reconnect_now() {
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
        connection_.session_state = "FORCED_RECONNECT";
        connection_.last_note = "用户请求立即重连";
    }
    log(MonitorLogLevel::kInfo, MonitorLogSource::kSession, "收到立即重连请求");
    on_reconnect_timer(ros::TimerEvent());
}
