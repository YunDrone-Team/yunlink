#include "backend/compare_backend.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "mapping/value_map.hpp"
#include "model/comparison.hpp"
#include "model/format.hpp"
#include "model/topic_defs.hpp"

namespace {

std::string level_token(const LogLevel level) {
    switch (level) {
    case LogLevel::kInfo:
        return "INFO";
    case LogLevel::kWarn:
        return "WARN";
    case LogLevel::kError:
        return "ERROR";
    }
    return "INFO";
}

std::string source_token(const LogSource source) {
    switch (source) {
    case LogSource::kCompare:
        return "Compare";
    case LogSource::kRos:
        return "ROS";
    case LogSource::kYunlink:
        return "Yunlink";
    case LogSource::kSession:
        return "Session";
    }
    return "Compare";
}

std::string make_log_key(const LogLevel level, const LogSource source, const std::string& line) {
    return level_token(level) + "|" + source_token(source) + "|" + line;
}

}  // namespace

CompareBackend::CompareBackend() : nh_(), pnh_("~"), topics_(make_default_topics()) {
    load_params();
    start_runtime();
    bind_runtime_diagnostics();
    bind_yunlink_subscribers();
    bind_ros_subscribers();
    setup_reconnect_timer();
}

std::unordered_map<std::string, TopicState> CompareBackend::snapshot_topics() const {
    std::lock_guard<std::mutex> lock(mu_);
    return topics_;
}

std::vector<LogEntry> CompareBackend::snapshot_logs() const {
    std::lock_guard<std::mutex> lock(mu_);
    return logs_;
}

double CompareBackend::align_window_ms() const {
    return align_window_ms_;
}

void CompareBackend::clear_logs() {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.clear();
    once_logs_.clear();
    throttled_logs_.clear();
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
    int log_limit_raw = static_cast<int>(log_limit_);
    pnh_.param<int>("log_limit", log_limit_raw, static_cast<int>(log_limit_));
    align_window_ms_ = std::max(1.0, align_window_ms_);
    history_limit_ = static_cast<size_t>(std::max(8, history_limit_raw_));
    log_limit_ = static_cast<size_t>(std::max(100, log_limit_raw));

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
        log(LogLevel::kError,
            LogSource::kYunlink,
            "Runtime 启动失败，ec=" + fmt_num(static_cast<int>(ec)));
        return;
    }
    log(LogLevel::kInfo, LogSource::kYunlink, "Runtime 已启动");
}

void CompareBackend::bind_runtime_diagnostics() {
    runtime_.event_bus().subscribe_link([this](const yunlink::LinkEvent& ev) {
        log(ev.is_up ? LogLevel::kInfo : LogLevel::kWarn,
            LogSource::kSession,
            "link " + std::string(ev.is_up ? "UP" : "DOWN") +
                " transport=" + fmt_num(static_cast<int>(ev.transport)) + " peer=" + ev.peer.id +
                " (" + ev.peer.ip + ":" + fmt_num(ev.peer.port) + ")");
    });

    runtime_.event_bus().subscribe_error([this](const yunlink::ErrorEvent& ev) {
        log(LogLevel::kError,
            LogSource::kYunlink,
            "error code=" + fmt_num(static_cast<int>(ev.code)) +
                " transport=" + fmt_num(static_cast<int>(ev.transport)) + " peer=" + ev.peer.id +
                " (" + ev.peer.ip + ":" + fmt_num(ev.peer.port) + ") msg=" + ev.message);
    });

    runtime_.event_bus().subscribe_envelope([this](const yunlink::EnvelopeEvent& ev) {
        if (ev.envelope.message_family != yunlink::MessageFamily::kStateSnapshot) {
            return;
        }
        log_once(LogLevel::kInfo,
                 LogSource::kYunlink,
                 "收到 state envelope type=" + fmt_num(ev.envelope.message_type) +
                     " session=" + fmt_num(ev.envelope.session_id) +
                     " from=" + fmt_num(static_cast<int>(ev.envelope.source.agent_type)) + ":" +
                     fmt_num(ev.envelope.source.agent_id));
    });
}

void CompareBackend::bind_yunlink_subscribers() {
    runtime_.state_subscriber().subscribe_local_odom(
        [this](const yunlink::TypedMessage<yunlink::LocalOdomSnapshot>& message) {
            std::unordered_map<std::string, std::string> values;
            fill_local_odom_from_yunlink(message.payload, values);
            log_once(LogLevel::kInfo, LogSource::kYunlink, "收到 snapshot: local_odom");
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
            log_once(LogLevel::kInfo, LogSource::kYunlink, "收到 snapshot: odom_state");
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
            log_once(LogLevel::kInfo, LogSource::kYunlink, "收到 snapshot: uav_control_state");
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
            log_once(LogLevel::kInfo, LogSource::kYunlink, "收到 snapshot: mavros_state");
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
            log_once(LogLevel::kInfo, LogSource::kYunlink, "收到 snapshot: px4_state");
            update_yunlink("px4_state",
                           std::move(values),
                           "session=" + fmt_num(message.envelope.session_id) +
                               " msg_id=" + fmt_num(message.envelope.message_id),
                           message.envelope.message_id,
                           message.envelope.created_at_ms);
        });

    log(LogLevel::kInfo, LogSource::kYunlink, "状态快照订阅器已就绪");
}

void CompareBackend::bind_ros_subscribers() {
    local_odom_sub_ =
        nh_.subscribe(topics_["local_odom"].ros_topic, 20, &CompareBackend::on_local_odom, this);
    odom_state_sub_ =
        nh_.subscribe(topics_["odom_state"].ros_topic, 20, &CompareBackend::on_odom_state, this);
    control_state_sub_ = nh_.subscribe(
        topics_["uav_control_state"].ros_topic, 20, &CompareBackend::on_control_state, this);
    mavros_state_sub_ = nh_.subscribe(
        topics_["mavros_state"].ros_topic, 20, &CompareBackend::on_mavros_state, this);
    px4_state_sub_ =
        nh_.subscribe(topics_["px4_state"].ros_topic, 20, &CompareBackend::on_px4_state, this);
    log(LogLevel::kInfo, LogSource::kRos, "原始话题订阅器已就绪");
}

void CompareBackend::setup_reconnect_timer() {
    reconnect_timer_ =
        nh_.createTimer(ros::Duration(1.0), &CompareBackend::on_reconnect_timer, this);
}
