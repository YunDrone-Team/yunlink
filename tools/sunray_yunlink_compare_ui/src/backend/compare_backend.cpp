#include "backend/compare_backend.hpp"

#include <algorithm>
#include <utility>

#include "mapping/value_map.hpp"
#include "model/comparison.hpp"
#include "model/format.hpp"
#include "model/topic_defs.hpp"

CompareBackend::CompareBackend() : nh_(), pnh_("~"), topics_(make_default_topics()) {
    load_params();
    start_runtime();
    bind_yunlink_subscribers();
    bind_ros_subscribers();
    setup_reconnect_timer();
}

std::unordered_map<std::string, TopicState> CompareBackend::snapshot_topics() const {
    std::lock_guard<std::mutex> lock(mu_);
    return topics_;
}

std::vector<std::string> CompareBackend::snapshot_logs() const {
    std::lock_guard<std::mutex> lock(mu_);
    return logs_;
}

double CompareBackend::align_window_ms() const {
    return align_window_ms_;
}

void CompareBackend::load_params() {
    pnh_.param<std::string>("remote_ip", remote_ip_, "127.0.0.1");
    pnh_.param<int>("remote_tcp_port", remote_tcp_port_, 9696);
    pnh_.param<int>("udp_bind_port", udp_bind_port_, 9797);
    pnh_.param<int>("udp_target_port", udp_target_port_, 9898);
    pnh_.param<int>("tcp_listen_port", tcp_listen_port_, 9797);
    pnh_.param<int>("agent_id", agent_id_, 1);
    pnh_.param<std::string>("agent_name", agent_name_, "uav");
    pnh_.param<std::string>("shared_secret", shared_secret_, "yunlink-default-secret");
    pnh_.param<std::string>("node_name", node_name_, "sunray_yunlink_compare_ui");
    pnh_.param<double>("align_window_ms", align_window_ms_, kDefaultAlignWindowMs);
    pnh_.param<int>("history_limit", history_limit_raw_, static_cast<int>(kDefaultHistoryLimit));
    align_window_ms_ = std::max(1.0, align_window_ms_);
    history_limit_ = static_cast<size_t>(std::max(8, history_limit_raw_));

    const std::string agent_key = "/" + agent_name_ + std::to_string(agent_id_);
    topics_["local_odom"].ros_topic = agent_key + "/sunray/localization/local_odom";
    topics_["odom_state"].ros_topic = agent_key + "/sunray/localization/odom_state";
    topics_["uav_control_state"].ros_topic = agent_key + "/sunray/uav_control/control_state";
    topics_["mavros_state"].ros_topic = agent_key + "/mavros/state";
    topics_["px4_state"].ros_topic = agent_key + "/sunray/px4_state";
}

void CompareBackend::start_runtime() {
    yunlink::RuntimeConfig cfg;
    cfg.udp_bind_port = clamp_port(udp_bind_port_);
    cfg.udp_target_port = clamp_port(udp_target_port_);
    cfg.tcp_listen_port = clamp_port(tcp_listen_port_);
    cfg.shared_secret = shared_secret_;
    cfg.self_identity.agent_type = yunlink::AgentType::kGroundStation;
    cfg.self_identity.agent_id = static_cast<uint32_t>(1000 + std::max(agent_id_, 0));
    cfg.self_identity.role = yunlink::EndpointRole::kObserver;
    const auto ec = runtime_.start(cfg);
    if (ec != yunlink::ErrorCode::kOk) {
        log("Yunlink Runtime 启动失败");
        return;
    }
    log("Yunlink Runtime 已启动");
}

void CompareBackend::bind_yunlink_subscribers() {
    runtime_.state_subscriber().subscribe_local_odom(
        [this](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_local_odom_from_yunlink(message.payload, values);
            update_yunlink("local_odom",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    runtime_.state_subscriber().subscribe_odom_state(
        [this](const yunlink::TypedMessage<yunlink::OdomStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_odom_state_from_yunlink(message.payload, values);
            update_yunlink("odom_state",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    runtime_.state_subscriber().subscribe_uav_control_state(
        [this](const yunlink::TypedMessage<yunlink::UavControlStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_control_state_from_yunlink(message.payload, values);
            update_yunlink("uav_control_state",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    runtime_.state_subscriber().subscribe_mavros_state(
        [this](const yunlink::TypedMessage<yunlink::MavrosStateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_mavros_state_from_yunlink(message.payload, values);
            update_yunlink("mavros_state",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    runtime_.state_subscriber().subscribe_px4_state(
        [this](const yunlink::TypedMessage<yunlink::Px4StateSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_px4_state_from_yunlink(message.payload, values);
            update_yunlink("px4_state",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    log("Yunlink 状态快照订阅器已就绪");
}

void CompareBackend::bind_ros_subscribers() {
    local_odom_sub_ = nh_.subscribe(
        topics_["local_odom"].ros_topic, 20, &CompareBackend::on_local_odom, this);
    odom_state_sub_ =
        nh_.subscribe(topics_["odom_state"].ros_topic, 20, &CompareBackend::on_odom_state, this);
    control_state_sub_ = nh_.subscribe(
        topics_["uav_control_state"].ros_topic, 20, &CompareBackend::on_control_state, this);
    mavros_state_sub_ = nh_.subscribe(
        topics_["mavros_state"].ros_topic, 20, &CompareBackend::on_mavros_state, this);
    px4_state_sub_ =
        nh_.subscribe(topics_["px4_state"].ros_topic, 20, &CompareBackend::on_px4_state, this);
    log("ROS 原始话题订阅器已就绪");
}

void CompareBackend::setup_reconnect_timer() {
    reconnect_timer_ =
        nh_.createTimer(ros::Duration(1.0), &CompareBackend::on_reconnect_timer, this);
}

uint16_t CompareBackend::clamp_port(int value) {
    if (value < 0) {
        return 0;
    }
    if (value > 65535) {
        return 65535;
    }
    return static_cast<uint16_t>(value);
}

void CompareBackend::on_reconnect_timer(const ros::TimerEvent&) {
    if (peer_ready_) {
        yunlink::SessionDescriptor desc{};
        if (!runtime_.session_server().describe_session(peer_id_, session_id_, &desc) ||
            desc.state != yunlink::SessionState::kActive) {
            peer_ready_ = false;
            session_id_ = 0;
            peer_id_.clear();
            log("Yunlink 会话已断开，准备重连");
        }
        return;
    }

    std::string peer_id;
    const auto ec =
        runtime_.tcp_clients().connect_peer(remote_ip_, clamp_port(remote_tcp_port_), &peer_id);
    if (ec != yunlink::ErrorCode::kOk) {
        log_throttle("连接 yunros 对端失败");
        return;
    }

    const uint64_t session_id = runtime_.session_client().open_active_session(peer_id, node_name_);
    if (session_id == 0) {
        log_throttle("打开 Yunlink 会话失败");
        return;
    }

    peer_ready_ = true;
    peer_id_ = peer_id;
    session_id_ = session_id;
    log("已连接 yunros，对端 peer_id=" + peer_id_ + "，session_id=" + fmt_num(session_id_));
}

void CompareBackend::on_local_odom(const nav_msgs::Odometry::ConstPtr& msg) {
    std::unordered_map<std::string, std::string> values;
    fill_local_odom_from_ros(*msg, values);
    update_ros("local_odom", std::move(values), msg->header.stamp);
}

void CompareBackend::on_odom_state(const sunray_msgs::OdomState::ConstPtr& msg) {
    std::unordered_map<std::string, std::string> values;
    fill_odom_state_from_ros(*msg, values);
    update_ros("odom_state", std::move(values), msg->header.stamp);
}

void CompareBackend::on_control_state(const sunray_msgs::UAVControlState::ConstPtr& msg) {
    std::unordered_map<std::string, std::string> values;
    fill_control_state_from_ros(*msg, values);
    update_ros("uav_control_state", std::move(values), msg->header.stamp);
}

void CompareBackend::on_mavros_state(const mavros_msgs::State::ConstPtr& msg) {
    std::unordered_map<std::string, std::string> values;
    fill_mavros_state_from_ros(*msg, values);
    update_ros("mavros_state", std::move(values), ros::Time::now());
}

void CompareBackend::on_px4_state(const sunray_msgs::Px4State::ConstPtr& msg) {
    std::unordered_map<std::string, std::string> values;
    fill_px4_state_from_ros(*msg, values);
    update_ros("px4_state", std::move(values), msg->header.stamp);
}

void CompareBackend::update_ros(const std::string& key,
                                std::unordered_map<std::string, std::string>&& values,
                                const ros::Time& stamp) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& topic = topics_.at(key);
    SnapshotSide snapshot;
    snapshot.values = std::move(values);
    snapshot.msg_stamp = stamp;
    snapshot.receive_time = ros::Time::now();
    topic.ros = snapshot;
    push_snapshot_history(topic.ros_history, topic.ros, history_limit_);
}

void CompareBackend::update_yunlink(const std::string& key,
                                    std::unordered_map<std::string, std::string>&& values,
                                    std::string note,
                                    uint64_t message_id,
                                    uint64_t created_at_ms) {
    std::lock_guard<std::mutex> lock(mu_);
    auto& topic = topics_.at(key);
    SnapshotSide snapshot;
    snapshot.values = std::move(values);
    snapshot.receive_time = ros::Time::now();
    snapshot.note = std::move(note);
    snapshot.message_id = message_id;
    snapshot.created_at_ms = created_at_ms;
    topic.yunlink = snapshot;
    push_snapshot_history(topic.yunlink_history, topic.yunlink, history_limit_);
}

void CompareBackend::log(const std::string& line) {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.push_back(line);
    if (logs_.size() > 16) {
        logs_.erase(logs_.begin());
    }
}

void CompareBackend::log_throttle(const std::string& line) {
    const ros::Time now = ros::Time::now();
    if ((now - last_log_time_).toSec() < 2.0) {
        return;
    }
    last_log_time_ = now;
    log(line);
}
