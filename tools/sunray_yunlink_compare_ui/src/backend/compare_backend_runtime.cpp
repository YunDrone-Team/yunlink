#include "backend/compare_backend.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "mapping/value_map.hpp"
#include "model/comparison.hpp"
#include "model/format.hpp"

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
            log(LogLevel::kWarn, LogSource::kSession, "会话已断开，准备重连");
        }
        return;
    }

    yunlink::SessionDescriptor active_session{};
    if (runtime_.session_server().find_active_session(&active_session)) {
        peer_ready_ = true;
        peer_id_ = active_session.peer.id;
        session_id_ = active_session.session_id;
        log(LogLevel::kInfo,
            LogSource::kSession,
            "复用现有会话，对端 peer_id=" + peer_id_ + "，session_id=" + fmt_num(session_id_));
        return;
    }

    std::string peer_id;
    const auto ec =
        runtime_.tcp_clients().connect_peer(remote_ip_, clamp_port(remote_tcp_port_), &peer_id);
    if (ec != yunlink::ErrorCode::kOk) {
        log_throttle(LogLevel::kWarn,
                     LogSource::kSession,
                     "连接 yunros 对端失败，ip=" + remote_ip_ +
                         " port=" + fmt_num(clamp_port(remote_tcp_port_)) +
                         " ec=" + fmt_num(static_cast<int>(ec)));
        return;
    }

    const uint64_t session_id = runtime_.session_client().open_active_session(peer_id, node_name_);
    if (session_id == 0) {
        log_throttle(LogLevel::kWarn,
                     LogSource::kSession,
                     "打开会话失败，peer_id=" + peer_id + " node=" + node_name_);
        return;
    }

    peer_ready_ = true;
    peer_id_ = peer_id;
    session_id_ = session_id;
    log(LogLevel::kInfo,
        LogSource::kSession,
        "已连接 yunros，对端 peer_id=" + peer_id_ + "，session_id=" + fmt_num(session_id_));
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

void CompareBackend::log(const LogLevel level, const LogSource source, const std::string& line) {
    std::lock_guard<std::mutex> lock(mu_);
    logs_.push_back(LogEntry{
        next_log_sequence_++,
        wall_time_ms(),
        level,
        source,
        line,
    });
    if (level == LogLevel::kError) {
        ROS_ERROR_STREAM("[compare_ui][" << source_token(source) << "] " << line);
    } else if (level == LogLevel::kWarn) {
        ROS_WARN_STREAM("[compare_ui][" << source_token(source) << "] " << line);
    } else {
        ROS_INFO_STREAM("[compare_ui][" << source_token(source) << "] " << line);
    }
    if (logs_.size() > log_limit_) {
        logs_.erase(logs_.begin());
    }
}

void CompareBackend::log_once(const LogLevel level,
                              const LogSource source,
                              const std::string& line) {
    {
        std::lock_guard<std::mutex> lock(mu_);
        if (!once_logs_.insert(make_log_key(level, source, line)).second) {
            return;
        }
    }
    log(level, source, line);
}

void CompareBackend::log_throttle(const LogLevel level,
                                  const LogSource source,
                                  const std::string& line) {
    const ros::Time now = ros::Time::now();
    const std::string key = make_log_key(level, source, line);
    {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = throttled_logs_.find(key);
        if (it != throttled_logs_.end() && (now - it->second).toSec() < 2.0) {
            return;
        }
        throttled_logs_[key] = now;
    }
    last_log_time_ = now;
    log(level, source, line);
}

uint64_t CompareBackend::wall_time_ms() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}
