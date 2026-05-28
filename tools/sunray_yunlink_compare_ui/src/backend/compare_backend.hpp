#ifndef SUNRAY_YUNLINK_COMPARE_UI_BACKEND_COMPARE_BACKEND_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_BACKEND_COMPARE_BACKEND_HPP

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

#include "model/topic_state.hpp"

class CompareBackend {
  public:
    CompareBackend();

    std::unordered_map<std::string, TopicState> snapshot_topics() const;
    std::vector<std::string> snapshot_logs() const;
    double align_window_ms() const;

  private:
    void load_params();
    void start_runtime();
    void bind_yunlink_subscribers();
    void bind_ros_subscribers();
    void setup_reconnect_timer();
    static uint16_t clamp_port(int value);

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
                        uint64_t created_at_ms);
    void log(const std::string& line);
    void log_throttle(const std::string& line);

    mutable std::mutex mu_;
    std::unordered_map<std::string, TopicState> topics_;
    std::vector<std::string> logs_;

    ros::NodeHandle nh_;
    ros::NodeHandle pnh_;
    ros::Subscriber local_odom_sub_;
    ros::Subscriber odom_state_sub_;
    ros::Subscriber control_state_sub_;
    ros::Subscriber mavros_state_sub_;
    ros::Subscriber px4_state_sub_;
    ros::Timer reconnect_timer_;
    ros::Time last_log_time_;

    yunlink::Runtime runtime_;
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
    int history_limit_raw_{static_cast<int>(kDefaultHistoryLimit)};
    uint64_t session_id_{0};
    bool peer_ready_{false};
    double align_window_ms_{kDefaultAlignWindowMs};
    size_t history_limit_{kDefaultHistoryLimit};
};

#endif  // SUNRAY_YUNLINK_COMPARE_UI_BACKEND_COMPARE_BACKEND_HPP
