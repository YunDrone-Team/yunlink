#ifndef SUNRAY_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP
#define SUNRAY_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include <mavros_msgs/State.h>
#include <nav_msgs/Odometry.h>
#include <ros/ros.h>
#include <sunray_msgs/OdomState.h>
#include <sunray_msgs/Px4State.h>
#include <sunray_msgs/UAVControlState.h>
#include <yunlink/yunlink.hpp>

#include "model/command_model.hpp"
#include "model/monitor_state.hpp"
#include "model/monitor_topics.hpp"

class AdvancedMonitorBackend {
  public:
    AdvancedMonitorBackend();
    ~AdvancedMonitorBackend();

    MonitorConnectionSnapshot snapshot_connection() const;
    std::unordered_map<std::string, MonitorTopicState> snapshot_topics() const;
    std::vector<MonitorLogEntry> snapshot_logs() const;
    std::vector<MonitorCommandHistoryEntry> snapshot_command_history() const;
    double state_timeout_sec() const;
    bool can_send_commands() const;
    void clear_logs();
    void request_reconnect_now();
    void stage_command(const MonitorCommandDraft& draft);
    void send_takeoff(const yunlink::TakeoffCommand& cmd);
    void send_land(const yunlink::LandCommand& cmd);
    void send_return(const yunlink::ReturnCommand& cmd);
    void send_goto(const yunlink::GotoCommand& cmd);
    void send_velocity_setpoint(const yunlink::VelocitySetpointCommand& cmd);
    void send_unsupported_command(const MonitorCommandDraft& draft);

  private:
    void load_params();
    void start_runtime();
    void bind_runtime_diagnostics();
    void bind_yunlink_subscribers();
    void bind_ros_subscribers();
    void bind_command_feedback();
    void setup_reconnect_timer();
    void update_config_snapshot();
    void request_command_authority_if_needed();
    yunlink::TargetSelector command_target() const;
    bool snapshot_send_context(std::string* peer_id, uint64_t* session_id) const;
    bool authority_active_unlocked() const;
    void mark_authority_pending_unlocked(uint64_t now_ms);
    void on_authority_status(const yunlink::TypedMessage<yunlink::AuthorityStatus>& message);
    void on_command_result(const yunlink::CommandResultView& message);
    void log_command_handle(const std::string& action,
                            const yunlink::CommandHandle& handle,
                            const std::string& detail);
    static std::string authority_state_label(yunlink::AuthorityState state);
    static std::string command_phase_label(yunlink::CommandPhase phase);
    static std::string command_kind_label(yunlink::CommandKind kind);
    static std::string error_code_label(yunlink::ErrorCode code);

    static uint16_t clamp_port(int value);
    static uint64_t wall_time_ms();

    void on_reconnect_timer(const ros::TimerEvent&);
    void on_local_odom(const nav_msgs::Odometry::ConstPtr& msg);
    void on_odom_state(const sunray_msgs::OdomState::ConstPtr& msg);
    void on_control_state(const sunray_msgs::UAVControlState::ConstPtr& msg);
    void on_mavros_state(const mavros_msgs::State::ConstPtr& msg);
    void on_px4_state(const sunray_msgs::Px4State::ConstPtr& msg);
    void update_ros(const std::string& key,
                    std::unordered_map<std::string, std::string>&& values,
                    const ros::Time& stamp);
    void update_yunlink(const std::string& key,
                        std::unordered_map<std::string, std::string>&& values,
                        std::string note,
                        uint64_t message_id,
                        uint64_t created_at_ms,
                        uint64_t session_id);
    void record_command_sent(const std::string& action,
                             const std::string& detail,
                             const yunlink::CommandHandle& handle);
    void update_command_result_history(const yunlink::CommandResultView& message);
    void apply_uav_control_state_history(const yunlink::UavControlStateSnapshot& snapshot);
    void refresh_command_timeouts(uint64_t now_ms);
    static bool control_cmd_matches_history(const MonitorCommandHistoryEntry& entry,
                                            const yunlink::UavControlCmdSnapshot& cmd);
    static uint8_t control_cmd_code_for_action(const std::string& action);
    void log(MonitorLogLevel level, MonitorLogSource source, const std::string& line);
    void log_throttle(MonitorLogLevel level, MonitorLogSource source, const std::string& line);

    mutable std::mutex mu_;
    std::unordered_map<std::string, MonitorTopicState> topics_;
    std::vector<MonitorLogEntry> logs_;
    std::vector<MonitorCommandHistoryEntry> command_history_;
    std::unordered_map<std::string, ros::Time> throttled_logs_;

    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;
    ros::Subscriber local_odom_sub_;
    ros::Subscriber odom_state_sub_;
    ros::Subscriber control_state_sub_;
    ros::Subscriber mavros_state_sub_;
    ros::Subscriber px4_state_sub_;
    ros::Timer reconnect_timer_;

    yunlink::Runtime runtime_;
    size_t link_token_{0};
    size_t error_token_{0};
    size_t command_result_token_{0};
    size_t authority_status_token_{0};
    std::vector<size_t> state_sub_tokens_;

    std::string remote_ip_;
    std::string shared_secret_;
    std::string node_name_;
    std::string agent_name_{"uav"};
    std::string peer_id_;
    int remote_tcp_port_{9696};
    int udp_bind_port_{9797};
    int udp_target_port_{9898};
    int tcp_listen_port_{9797};
    int agent_id_{1};
    int log_limit_raw_{500};
    size_t log_limit_{500};
    double state_timeout_sec_{2.0};
    uint32_t authority_ttl_ms_{3000};
    uint64_t authority_expires_at_ms_{0};
    uint64_t authority_request_at_ms_{0};
    uint64_t next_command_sequence_{1};
    bool authority_pending_{false};
    uint64_t session_id_{0};
    uint64_t next_log_sequence_{1};
    bool runtime_started_{false};
    bool peer_ready_{false};
    MonitorConnectionSnapshot connection_;
    size_t command_history_limit_{32};
    uint64_t command_timeout_ms_{3000};
};

#endif  // SUNRAY_ADVANCED_MONITOR_BACKEND_ADVANCED_MONITOR_BACKEND_HPP
