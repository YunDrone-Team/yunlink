#include "backend/advanced_monitor_backend.hpp"

AdvancedMonitorBackend::AdvancedMonitorBackend() : AdvancedMonitorBackend(Config()) {}

AdvancedMonitorBackend::AdvancedMonitorBackend(Config config)
    : topics_(make_default_monitor_topics()), remote_ip_(std::move(config.remote_ip)),
      shared_secret_(std::move(config.shared_secret)), node_name_(std::move(config.node_name)),
      agent_name_(std::move(config.agent_name)), remote_tcp_port_(config.remote_tcp_port),
      udp_bind_port_(config.udp_bind_port), udp_target_port_(config.udp_target_port),
      tcp_listen_port_(config.tcp_listen_port), agent_id_(config.agent_id),
      log_limit_raw_(config.log_limit), discovery_port_(config.discovery_port),
      authority_ttl_ms_(static_cast<uint32_t>(config.authority_ttl_ms)),
      command_history_limit_(static_cast<size_t>(config.command_history_limit)),
      command_timeout_ms_(static_cast<uint64_t>(config.command_timeout_ms)),
      discovery_target_ip_(std::move(config.discovery_target_ip)),
      started_at_steady_(std::chrono::steady_clock::now()) {
    update_config_snapshot();
    start_runtime();
    start_discovery_listener();
    bind_runtime_diagnostics();
    bind_packet_trace();
    bind_yunlink_subscribers();
    bind_command_feedback();
    bind_system_service_feedback();
    bind_configuration_feedback();
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
    if (feature_list_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(feature_list_response_token_);
    }
    if (feature_get_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(feature_get_response_token_);
    }
    if (feature_start_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(feature_start_response_token_);
    }
    if (feature_stop_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(feature_stop_response_token_);
    }
    if (runtime_log_list_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(runtime_log_list_response_token_);
    }
    if (runtime_log_read_response_token_ != 0) {
        runtime_.system_service_subscriber().unsubscribe(runtime_log_read_response_token_);
    }
    for (size_t token : {config_list_response_token_,
                         config_describe_response_token_,
                         config_get_response_token_,
                         config_patch_response_token_,
                         config_apply_response_token_}) {
        if (token != 0) {
            runtime_.configuration_service_subscriber().unsubscribe(token);
        }
    }
    if (link_token_ != 0) {
        runtime_.event_bus().unsubscribe(link_token_);
    }
    if (error_token_ != 0) {
        runtime_.event_bus().unsubscribe(error_token_);
    }
    if (packet_trace_token_ != 0) {
        runtime_.event_subscriber().unsubscribe(packet_trace_token_);
    }
    discovery_listener_.stop();
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

std::vector<yunlink::PacketTraceRecord> AdvancedMonitorBackend::snapshot_packet_traces() const {
    std::lock_guard<std::mutex> lock(mu_);
    return std::vector<yunlink::PacketTraceRecord>(packet_traces_.begin(), packet_traces_.end());
}

std::vector<yunlink::PacketTraceRecord>
AdvancedMonitorBackend::snapshot_packet_traces_since(uint64_t trace_id,
                                                     uint64_t retained_first_trace_id,
                                                     bool* reset_required) const {
    std::lock_guard<std::mutex> lock(mu_);
    if (reset_required != nullptr) {
        *reset_required = !packet_traces_.empty() && retained_first_trace_id != 0 &&
                          packet_traces_.front().trace_id > retained_first_trace_id;
    }
    if (reset_required != nullptr && *reset_required) {
        return std::vector<yunlink::PacketTraceRecord>(packet_traces_.begin(), packet_traces_.end());
    }
    std::vector<yunlink::PacketTraceRecord> out;
    for (const auto& record : packet_traces_) {
        if (record.trace_id > trace_id) {
            out.push_back(record);
        }
    }
    return out;
}

std::vector<MonitorCommandHistoryEntry> AdvancedMonitorBackend::snapshot_command_history() const {
    std::lock_guard<std::mutex> lock(mu_);
    return command_history_;
}

MonitorSystemServiceState AdvancedMonitorBackend::snapshot_system_services() const {
    std::lock_guard<std::mutex> lock(mu_);
    return system_services_;
}

MonitorConfigurationState AdvancedMonitorBackend::snapshot_configuration() const {
    std::lock_guard<std::mutex> lock(mu_);
    return configuration_;
}

std::vector<MonitorSystemServiceHistoryEntry>
AdvancedMonitorBackend::snapshot_system_service_history() const {
    std::lock_guard<std::mutex> lock(mu_);
    return system_service_history_;
}

bool AdvancedMonitorBackend::can_send_commands() const {
    std::lock_guard<std::mutex> lock(mu_);
    return runtime_started_ && peer_ready_ && session_id_ != 0 && authority_active_unlocked();
}

void AdvancedMonitorBackend::set_debug_stream_enabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mu_);
    debug_stream_enabled_ = enabled;
}

void AdvancedMonitorBackend::clear_logs() {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.clear();
    command_status_log_indices_.clear();
    last_log_key_.clear();
    has_bridge_runtime_diagnostic_level_ = false;
    last_bridge_runtime_diagnostic_ok_ = false;
}

void AdvancedMonitorBackend::clear_packet_traces() {
    std::lock_guard<std::mutex> lock(mu_);
    packet_traces_.clear();
    packet_trace_bytes_ = 0;
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
    log(MonitorLogLevel::kInfo, MonitorLogSource::kConnection, "收到立即重连请求");
    poll_runtime();
}
