#ifndef YUNLINK_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP
#define YUNLINK_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <chrono>
#include <deque>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <yunlink/yunlink.hpp>

#include "model/command_model.hpp"
#include "model/configuration/model.hpp"
#include "model/discovery/discovery_device.hpp"
#include "model/monitor_state.hpp"
#include "model/monitor_topics.hpp"
#include "model/system_service_model.hpp"

class AdvancedMonitorBackend {
  public:
    struct Config {
        std::string remote_ip;
        std::string shared_secret{"yunlink-default-secret"};
        std::string node_name{"yunlink_advanced_monitor"};
        std::string agent_name{"uav"};
        int remote_tcp_port{0};
        int udp_bind_port{9797};
        int udp_target_port{9898};
        int tcp_listen_port{9797};
        int agent_id{1};
        int log_limit{500};
        int authority_ttl_ms{3000};
        int command_history_limit{32};
        int command_timeout_ms{3000};
        int discovery_port{9966};
        std::string discovery_target_ip{"255.255.255.255"};
    };

    AdvancedMonitorBackend();
    explicit AdvancedMonitorBackend(Config config);
    ~AdvancedMonitorBackend();

    void poll_runtime();
    MonitorConnectionSnapshot snapshot_connection() const;
    std::vector<yunlink::PacketTraceRecord> snapshot_packet_traces() const;
    std::vector<yunlink::PacketTraceRecord>
    snapshot_packet_traces_since(uint64_t trace_id,
                                 uint64_t retained_first_trace_id,
                                 bool* reset_required) const;
    std::unordered_map<std::string, MonitorTopicState> snapshot_topics() const;
    std::vector<MonitorLogEntry> snapshot_logs() const;
    std::vector<DiscoveryDevice> snapshot_discovery_devices() const;
    std::vector<MonitorCommandHistoryEntry> snapshot_command_history() const;
    MonitorSystemServiceState snapshot_system_services() const;
    MonitorConfigurationState snapshot_configuration() const;
    std::vector<MonitorSystemServiceHistoryEntry> snapshot_system_service_history() const;
    bool can_send_commands() const;
    void set_debug_stream_enabled(bool enabled);
    void clear_logs();
    void clear_packet_traces();
    void request_discovery_scan();
    void request_reconnect_now();
    bool connect_to_discovered_device(const std::string& dedupe_key);
    bool disconnect_current_device();
    std::string selected_discovery_device_key() const;
    void send_takeoff(const yunlink::TakeoffCommand& cmd);
    void send_land(const yunlink::LandCommand& cmd);
    void send_return(const yunlink::ReturnCommand& cmd);
    void send_goto(const yunlink::GotoCommand& cmd);
    void send_velocity_setpoint(const yunlink::VelocitySetpointCommand& cmd);
    void request_feature_list();
    void request_feature_get(const std::string& feature_name);
    void request_feature_start(const std::string& feature_name,
                               const std::vector<std::string>& override_args,
                               bool restart_if_running,
                               bool start_with_terminal);
    void request_feature_stop(const std::string& feature_name, bool force);
    void request_config_resource_list();
    void request_config_resource_describe(const std::string& resource_id);
    void request_config_resource_get(const std::string& resource_id);
    void request_config_resource_patch(const std::string& resource_id,
                                       const std::string& expected_revision,
                                       const std::vector<yunlink::ConfigFieldValue>& updates,
                                       bool validate_only);
    void request_config_resource_apply(const std::string& resource_id,
                                       const std::string& expected_revision);

  private:
    void load_params();
    void start_runtime();
    void bind_runtime_diagnostics();
    void bind_packet_trace();
    void bind_yunlink_subscribers();
    void bind_command_feedback();
    void bind_system_service_feedback();
    void bind_configuration_feedback();
    void start_discovery_listener();
    void setup_reconnect_timer();
    void update_config_snapshot();
    void poll_discovery();
    void poll_discovery_scan();
    void request_command_authority_if_needed();
    yunlink::TargetSelector command_target() const;
    yunlink::TargetSelector system_service_target() const;
    yunlink::TargetSelector configuration_service_target() const;
    bool snapshot_send_context(std::string* peer_id, uint64_t* session_id) const;
    bool authority_active_unlocked() const;
    void mark_authority_pending_unlocked(uint64_t now_ms);
    void on_authority_status(const yunlink::TypedMessage<yunlink::AuthorityStatus>& message);
    void on_command_result(const yunlink::CommandResultView& message);
    void on_command_execution_status(
        const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message);
    void log_command_handle(const std::string& action,
                            const yunlink::CommandHandle& handle,
                            const std::string& detail);
    static std::string authority_state_label(yunlink::AuthorityState state);
    static std::string command_phase_label(yunlink::CommandPhase phase);
    static std::string command_kind_label(yunlink::CommandKind kind);
    static std::string command_execution_state_label(uint8_t state);
    static std::string error_code_label(yunlink::ErrorCode code);

    void on_packet_trace(const yunlink::PacketTraceRecord& record);
    void trim_packet_traces_unlocked();
    static uint16_t clamp_port(int value);
    static uint64_t wall_time_ms();
    static uint64_t steady_time_ms();

    void update_yunlink(const std::string& key,
                        std::unordered_map<std::string, std::string>&& values,
                        std::string note,
                        uint64_t source_stamp_ns,
                        uint64_t message_id,
                        uint64_t created_at_ms,
                        uint64_t session_id);
    void record_command_sent(const std::string& action,
                             const std::string& detail,
                             const yunlink::CommandHandle& handle);
    void update_command_result_history(const yunlink::CommandResultView& message);
    void update_command_execution_history(
        const yunlink::TypedMessage<yunlink::CommandExecutionStatusSnapshot>& message);
    void refresh_command_timeouts(uint64_t now_ms);
    void
    on_feature_list_response(const yunlink::TypedMessage<yunlink::FeatureListResponse>& message);
    void on_feature_get_response(const yunlink::TypedMessage<yunlink::FeatureGetResponse>& message);
    void
    on_feature_start_response(const yunlink::TypedMessage<yunlink::FeatureStartResponse>& message);
    void
    on_feature_stop_response(const yunlink::TypedMessage<yunlink::FeatureStopResponse>& message);
    void record_system_service_request(const std::string& action,
                                       const std::string& feature_name,
                                       const yunlink::SystemServiceHandle& handle);
    void refresh_system_service_timeouts(uint64_t now_ms);
    void on_config_resource_list_response(
        const yunlink::TypedMessage<yunlink::ConfigResourceListResponse>& message);
    void on_config_resource_describe_response(
        const yunlink::TypedMessage<yunlink::ConfigResourceDescribeResponse>& message);
    void on_config_resource_get_response(
        const yunlink::TypedMessage<yunlink::ConfigResourceGetResponse>& message);
    void on_config_resource_patch_response(
        const yunlink::TypedMessage<yunlink::ConfigResourcePatchResponse>& message);
    void on_config_resource_apply_response(
        const yunlink::TypedMessage<yunlink::ConfigResourceApplyResponse>& message);
    void log(MonitorLogLevel level, MonitorLogSource source, const std::string& line);
    void log_debug(MonitorLogSource source, const std::string& line);
    static std::string
    make_semantic_log_key(MonitorLogLevel level, MonitorLogSource source, const std::string& line);
    static bool should_merge_repeated_log(MonitorLogSource source, const std::string& line);
    void on_sunray_runtime_diagnostic(
        const yunlink::TypedMessage<yunlink::SunrayRuntimeDiagnosticSnapshot>& message);
    void update_discovery_snapshot_unlocked(const DiscoveryDevice& device);
    static std::string make_discovery_key(const std::string& endpoint_id);

    mutable std::mutex mu_;
    std::unordered_map<std::string, MonitorTopicState> topics_;
    std::unordered_map<std::string, DiscoveryDevice> discovery_devices_;
    std::vector<MonitorLogEntry> logs_;
    std::deque<yunlink::PacketTraceRecord> packet_traces_;
    std::vector<MonitorCommandHistoryEntry> command_history_;
    std::unordered_map<std::string, size_t> command_status_log_indices_;
    std::string last_log_key_;
    bool has_bridge_runtime_diagnostic_level_{false};
    bool last_bridge_runtime_diagnostic_ok_{false};

    yunlink::Runtime runtime_;
    yunlink::EndpointListener discovery_listener_;
    size_t link_token_{0};
    size_t error_token_{0};
    size_t packet_trace_token_{0};
    size_t command_result_token_{0};
    size_t authority_status_token_{0};
    size_t feature_list_response_token_{0};
    size_t feature_get_response_token_{0};
    size_t feature_start_response_token_{0};
    size_t feature_stop_response_token_{0};
    size_t config_list_response_token_{0};
    size_t config_describe_response_token_{0};
    size_t config_get_response_token_{0};
    size_t config_patch_response_token_{0};
    size_t config_apply_response_token_{0};
    std::vector<size_t> state_sub_tokens_;

    std::string remote_ip_;
    std::string shared_secret_;
    std::string node_name_;
    std::string agent_name_{"uav"};
    std::string peer_id_;
    int remote_tcp_port_{0};
    int udp_bind_port_{9797};
    int udp_target_port_{9898};
    int tcp_listen_port_{9797};
    int agent_id_{1};
    int log_limit_raw_{500};
    int discovery_port_{9966};
    size_t log_limit_{500};
    size_t packet_trace_limit_{500};
    size_t packet_trace_max_bytes_{8 * 1024 * 1024};
    size_t packet_trace_bytes_{0};
    uint32_t authority_ttl_ms_{3000};
    uint64_t authority_expires_at_ms_{0};
    uint64_t authority_request_at_ms_{0};
    uint64_t next_command_sequence_{1};
    uint64_t next_system_service_sequence_{1};
    bool authority_pending_{false};
    bool debug_stream_enabled_{false};
    uint64_t session_id_{0};
    uint64_t next_log_sequence_{1};
    bool runtime_started_{false};
    bool discovery_listener_started_{false};
    uint64_t discovery_scan_nonce_{0};
    uint64_t discovery_scan_next_send_ms_{0};
    uint64_t discovery_scan_expires_ms_{0};
    int discovery_scan_remaining_{0};
    bool peer_ready_{false};
    std::string selected_discovery_key_;
    MonitorConnectionSnapshot connection_;
    size_t command_history_limit_{32};
    uint64_t command_timeout_ms_{3000};
    MonitorSystemServiceState system_services_;
    MonitorConfigurationState configuration_;
    std::unordered_map<uint64_t, bool> config_patch_validate_requests_;
    std::vector<MonitorSystemServiceHistoryEntry> system_service_history_;
    size_t system_service_history_limit_{32};
    uint64_t system_service_timeout_ms_{5000};
    std::chrono::steady_clock::time_point started_at_steady_;
    std::string discovery_target_ip_{"255.255.255.255"};
};

#endif  // YUNLINK_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP
