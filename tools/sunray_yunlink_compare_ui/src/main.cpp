#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <deque>
#include <functional>
#include <limits>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QSplitter>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <mavros_msgs/State.h>
#include <nav_msgs/Odometry.h>
#include <ros/ros.h>
#include <sunray_msgs/OdomState.h>
#include <sunray_msgs/Px4State.h>
#include <sunray_msgs/UAVControlState.h>
#include <yunlink/yunlink.hpp>

namespace {

template <typename T>
double to_double(T value) {
    return static_cast<double>(value);
}

struct FieldView {
    std::string label;
    std::string ros_value;
    std::string yunlink_value;
    std::string delta_text;
    bool comparable = false;
    bool equal = false;
};

struct SnapshotSide {
    std::unordered_map<std::string, std::string> values;
    ros::Time msg_stamp;
    ros::Time receive_time;
    std::string note;
    uint64_t message_id = 0;
    uint64_t created_at_ms = 0;
};

struct TopicState {
    std::string key;
    std::string title;
    std::string ros_topic;
    std::string yunlink_name;
    std::vector<std::string> uncovered_fields;
    std::vector<FieldView> rows;
    SnapshotSide ros;
    SnapshotSide yunlink;
    std::deque<SnapshotSide> ros_history;
    std::deque<SnapshotSide> yunlink_history;
};

struct ComparisonSelection {
    SnapshotSide ros;
    SnapshotSide yunlink;
    bool matched = false;
    double receive_dt_ms = std::numeric_limits<double>::quiet_NaN();
};

constexpr size_t kDefaultHistoryLimit = 120;
constexpr double kDefaultAlignWindowMs = 80.0;
constexpr double kDefaultFloatEpsilon = 1e-4;
constexpr double kDynamicFloatEpsilon = 1e-3;

std::string fmt_float(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(6);
    ss << value;
    return ss.str();
}

std::string fmt_bool(bool value) {
    return value ? "true" : "false";
}

template <typename T>
std::string fmt_num(T value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

std::string fmt_ros_time(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << stamp.toSec();
    return ss.str();
}

std::string fmt_age(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    const double age = std::max(0.0, (ros::Time::now() - stamp).toSec());
    return fmt_float(age);
}

std::string fmt_ms(double value_ms) {
    if (std::isnan(value_ms)) {
        return "--";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << value_ms;
    return ss.str();
}

bool has_snapshot(const SnapshotSide& side) {
    return !side.values.empty();
}

double receive_dt_ms(const ros::Time& lhs, const ros::Time& rhs) {
    if (lhs.isZero() || rhs.isZero()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::fabs((lhs - rhs).toSec() * 1000.0);
}

bool has_prefix(const std::string& value, const std::string& prefix) {
    return value.size() >= prefix.size() && value.compare(0, prefix.size(), prefix) == 0;
}

bool equal_text(const std::string& lhs, const std::string& rhs) {
    return lhs == rhs;
}

bool equal_float(const std::string& lhs, const std::string& rhs, double eps = 1e-4) {
    try {
        const double lv = std::stod(lhs);
        const double rv = std::stod(rhs);
        return std::fabs(lv - rv) <= eps;
    } catch (...) {
        return false;
    }
}

std::string delta_float(const std::string& lhs, const std::string& rhs) {
    try {
        const double lv = std::stod(lhs);
        const double rv = std::stod(rhs);
        return fmt_float(rv - lv);
    } catch (...) {
        return "--";
    }
}

double field_epsilon(const std::string& topic_key, const std::string& field_key) {
    if (topic_key == "local_odom") {
        if (has_prefix(field_key, "position_m.") || has_prefix(field_key, "linear_velocity_mps.")) {
            return kDynamicFloatEpsilon;
        }
    }
    if (topic_key == "px4_state") {
        if (has_prefix(field_key, "local_position_m.") || has_prefix(field_key, "local_velocity_mps.") ||
            field_key == "yaw_setpoint_rad" || field_key == "yaw_rate_setpoint_radps") {
            return kDynamicFloatEpsilon;
        }
    }
    return kDefaultFloatEpsilon;
}

void push_snapshot_history(std::deque<SnapshotSide>& history,
                           const SnapshotSide& snapshot,
                           size_t history_limit) {
    history.push_back(snapshot);
    while (history.size() > history_limit) {
        history.pop_front();
    }
}

ComparisonSelection make_latest_selection(const TopicState& topic) {
    ComparisonSelection selection;
    selection.ros = topic.ros;
    selection.yunlink = topic.yunlink;
    selection.matched = has_snapshot(selection.ros) && has_snapshot(selection.yunlink);
    if (selection.matched) {
        selection.receive_dt_ms = receive_dt_ms(selection.ros.receive_time, selection.yunlink.receive_time);
    }
    return selection;
}

ComparisonSelection make_aligned_selection(const TopicState& topic, double align_window_ms) {
    ComparisonSelection selection;
    if (!topic.yunlink_history.empty()) {
        selection.yunlink = topic.yunlink_history.back();
    }
    if (!has_snapshot(selection.yunlink) || topic.ros_history.empty()) {
        return selection;
    }

    const SnapshotSide* best_ros = nullptr;
    double best_dt_ms = std::numeric_limits<double>::infinity();
    for (auto it = topic.ros_history.rbegin(); it != topic.ros_history.rend(); ++it) {
        if (!has_snapshot(*it)) {
            continue;
        }
        const double current_dt_ms = receive_dt_ms(it->receive_time, selection.yunlink.receive_time);
        if (std::isnan(current_dt_ms)) {
            continue;
        }
        if (current_dt_ms < best_dt_ms) {
            best_dt_ms = current_dt_ms;
            best_ros = &(*it);
        }
    }

    if (best_ros == nullptr || best_dt_ms > align_window_ms) {
        selection.receive_dt_ms = best_dt_ms;
        return selection;
    }

    selection.ros = *best_ros;
    selection.matched = true;
    selection.receive_dt_ms = best_dt_ms;
    return selection;
}

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value) {
    values[key] = value;
}

template <typename T>
void set_numeric(std::unordered_map<std::string, std::string>& values,
                 const std::string& key,
                 T value) {
    values[key] = fmt_num(value);
}

void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value) {
    values[key] = fmt_float(value);
}

void fill_local_odom_from_ros(const nav_msgs::Odometry& msg,
                              std::unordered_map<std::string, std::string>& values) {
    set_float(values, "position_m.x", msg.pose.pose.position.x);
    set_float(values, "position_m.y", msg.pose.pose.position.y);
    set_float(values, "position_m.z", msg.pose.pose.position.z);
    set_float(values, "orientation_x", msg.pose.pose.orientation.x);
    set_float(values, "orientation_y", msg.pose.pose.orientation.y);
    set_float(values, "orientation_z", msg.pose.pose.orientation.z);
    set_float(values, "orientation_w", msg.pose.pose.orientation.w);
    set_float(values, "linear_velocity_mps.x", msg.twist.twist.linear.x);
    set_float(values, "linear_velocity_mps.y", msg.twist.twist.linear.y);
    set_float(values, "linear_velocity_mps.z", msg.twist.twist.linear.z);
}

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_float(values, "position_m.x", msg.position_m.x);
    set_float(values, "position_m.y", msg.position_m.y);
    set_float(values, "position_m.z", msg.position_m.z);
    set_float(values, "orientation_x", msg.orientation_x);
    set_float(values, "orientation_y", msg.orientation_y);
    set_float(values, "orientation_z", msg.orientation_z);
    set_float(values, "orientation_w", msg.orientation_w);
    set_float(values, "linear_velocity_mps.x", msg.linear_velocity_mps.x);
    set_float(values, "linear_velocity_mps.y", msg.linear_velocity_mps.y);
    set_float(values, "linear_velocity_mps.z", msg.linear_velocity_mps.z);
}

void fill_odom_state_from_ros(const sunray_msgs::OdomState& msg,
                              std::unordered_map<std::string, std::string>& values) {
    set_numeric(values, "external_source", msg.external_source);
    set_value(values, "subtopic_name_external_odom", msg.subtopic_name_external_odom);
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_float(values, "odometry_update_hz", msg.odometry_update_hz);
    set_value(values,
              "subtopic_name_external_relocalization",
              msg.subtopic_name_external_relocalization);
    set_value(values, "pubtopic_name_local_odom", msg.pubtopic_name_local_odom);
    set_value(values, "pubtopic_name_global_odom", msg.pubtopic_name_global_odom);
    set_value(values, "world_frame_name", msg.world_frame_name);
    set_value(values, "global_frame_name", msg.global_frame_name);
    set_value(values, "local_frame_name", msg.local_frame_name);
    set_value(values, "base_frame_name", msg.base_frame_name);
}

void fill_odom_state_from_yunlink(const yunlink::OdomStateSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values) {
    set_numeric(values, "external_source", msg.external_source);
    set_value(values, "subtopic_name_external_odom", msg.subtopic_name_external_odom);
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_float(values, "odometry_update_hz", msg.odometry_update_hz);
    set_value(values,
              "subtopic_name_external_relocalization",
              msg.subtopic_name_external_relocalization);
    set_value(values, "pubtopic_name_local_odom", msg.pubtopic_name_local_odom);
    set_value(values, "pubtopic_name_global_odom", msg.pubtopic_name_global_odom);
    set_value(values, "world_frame_name", msg.world_frame_name);
    set_value(values, "global_frame_name", msg.global_frame_name);
    set_value(values, "local_frame_name", msg.local_frame_name);
    set_value(values, "base_frame_name", msg.base_frame_name);
}

void fill_control_state_from_ros(const sunray_msgs::UAVControlState& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_numeric(values, "controller_types", msg.controller_types);
    set_float(values, "takeoff_relative_height_m", msg.takeoff_relative_height);
    set_float(values, "takeoff_max_velocity_mps", msg.takeoff_max_velocity);
    set_numeric(values, "land_type", msg.land_type);
    set_float(values, "land_max_velocity_mps", msg.land_max_velocity);
    set_float(values, "home_point_m.x", msg.home_point.x);
    set_float(values, "home_point_m.y", msg.home_point.y);
    set_float(values, "home_point_m.z", msg.home_point.z);
    set_numeric(values, "control_state", msg.control_state);
    set_numeric(values, "last_control_cmd", msg.last_cmd.control_cmd);
    set_numeric(values, "last_cmd_source", msg.last_cmd.cmd_source);
    set_value(values, "odometry_lost", fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_float(values, "self_odom_z_m", msg.self_odom.pose.pose.position.z);
}

void fill_control_state_from_yunlink(const yunlink::UavControlStateSnapshot& msg,
                                     std::unordered_map<std::string, std::string>& values) {
    set_numeric(values, "controller_types", msg.controller_types);
    set_float(values, "takeoff_relative_height_m", msg.takeoff_relative_height_m);
    set_float(values, "takeoff_max_velocity_mps", msg.takeoff_max_velocity_mps);
    set_numeric(values, "land_type", msg.land_type);
    set_float(values, "land_max_velocity_mps", msg.land_max_velocity_mps);
    set_float(values, "home_point_m.x", msg.home_point_m.x);
    set_float(values, "home_point_m.y", msg.home_point_m.y);
    set_float(values, "home_point_m.z", msg.home_point_m.z);
    set_numeric(values, "control_state", msg.control_state);
    set_numeric(values, "last_control_cmd", msg.last_control_cmd);
    set_numeric(values, "last_cmd_source", msg.last_cmd_source);
    set_value(values, "odometry_lost", fmt_bool(msg.odometry_lost));
    set_value(values, "odometry_valid", fmt_bool(msg.odometry_valid));
    set_float(values, "self_odom_z_m", msg.self_odom_z_m);
}

void fill_mavros_state_from_ros(const mavros_msgs::State& msg,
                                std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_value(values, "guided", fmt_bool(msg.guided));
    set_value(values, "mode_name", msg.mode);
    set_numeric(values, "system_status", msg.system_status);
}

void fill_mavros_state_from_yunlink(const yunlink::MavrosStateSnapshot& msg,
                                    std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_value(values, "guided", fmt_bool(msg.guided));
    set_value(values, "mode_name", msg.mode_name);
    set_numeric(values, "system_status", msg.system_status);
}

void fill_px4_state_from_ros(const sunray_msgs::Px4State& msg,
                             std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_numeric(values, "flight_mode", msg.flight_mode);
    set_value(values, "flight_mode_name", fmt_num(msg.flight_mode));
    set_numeric(values, "system_status", msg.system_status);
    set_numeric(values, "landed_state", msg.landed_state);
    set_float(values, "battery_voltage_v", msg.battery_voltage_v);
    set_float(values, "battery_current_a", msg.battery_current_a);
    set_float(values, "battery_percentage", msg.battery_percentage);
    set_float(values, "local_position_m.x", msg.local_pose.position.x);
    set_float(values, "local_position_m.y", msg.local_pose.position.y);
    set_float(values, "local_position_m.z", msg.local_pose.position.z);
    set_float(values, "local_velocity_mps.x", msg.local_velocity.linear.x);
    set_float(values, "local_velocity_mps.y", msg.local_velocity.linear.y);
    set_float(values, "local_velocity_mps.z", msg.local_velocity.linear.z);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude);
    set_float(values, "longitude_deg", msg.longitude);
    set_float(values, "altitude_m", msg.altitude);
}

void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values) {
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "rc_available", fmt_bool(msg.rc_available));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_numeric(values, "flight_mode", msg.flight_mode);
    set_value(values, "flight_mode_name", msg.flight_mode_name);
    set_numeric(values, "system_status", msg.system_status);
    set_numeric(values, "landed_state", msg.landed_state);
    set_float(values, "battery_voltage_v", msg.battery_voltage_v);
    set_float(values, "battery_current_a", msg.battery_current_a);
    set_float(values, "battery_percentage", msg.battery_percentage);
    set_float(values, "local_position_m.x", msg.local_position_m.x);
    set_float(values, "local_position_m.y", msg.local_position_m.y);
    set_float(values, "local_position_m.z", msg.local_position_m.z);
    set_float(values, "local_velocity_mps.x", msg.local_velocity_mps.x);
    set_float(values, "local_velocity_mps.y", msg.local_velocity_mps.y);
    set_float(values, "local_velocity_mps.z", msg.local_velocity_mps.z);
    set_float(values, "yaw_setpoint_rad", msg.yaw_setpoint_rad);
    set_float(values, "yaw_rate_setpoint_radps", msg.yaw_rate_setpoint_radps);
    set_numeric(values, "satellites", msg.satellites);
    set_numeric(values, "gps_status", msg.gps_status);
    set_numeric(values, "gps_service", msg.gps_service);
    set_float(values, "latitude_deg", msg.latitude_deg);
    set_float(values, "longitude_deg", msg.longitude_deg);
    set_float(values, "altitude_m", msg.altitude_m);
}

class CompareBackend {
  public:
    CompareBackend() : nh_(), pnh_("~") {
        init_topics();
        load_params();
        start_runtime();
        bind_yunlink_subscribers();
        bind_ros_subscribers();
        setup_reconnect_timer();
    }

    std::unordered_map<std::string, TopicState> snapshot_topics() const {
        std::lock_guard<std::mutex> lock(mu_);
        return topics_;
    }

    std::vector<std::string> snapshot_logs() const {
        std::lock_guard<std::mutex> lock(mu_);
        return logs_;
    }

    double align_window_ms() const {
        return align_window_ms_;
    }

  private:
    void init_topics() {
        TopicState local_odom;
        local_odom.key = "local_odom";
        local_odom.title = "local_odom";
        local_odom.ros_topic = "/uav1/sunray/localization/local_odom";
        local_odom.yunlink_name = "LocalOdomSnapshot";
        local_odom.rows = {
            {"位置 X | position_m.x"},
            {"位置 Y | position_m.y"},
            {"位置 Z | position_m.z"},
            {"姿态四元数 X | orientation_x"},
            {"姿态四元数 Y | orientation_y"},
            {"姿态四元数 Z | orientation_z"},
            {"姿态四元数 W | orientation_w"},
            {"线速度 X | linear_velocity_mps.x"},
            {"线速度 Y | linear_velocity_mps.y"},
            {"线速度 Z | linear_velocity_mps.z"},
        };

        TopicState odom_state;
        odom_state.key = "odom_state";
        odom_state.title = "odom_state";
        odom_state.ros_topic = "/uav1/sunray/localization/odom_state";
        odom_state.yunlink_name = "OdomStateSnapshot";
        odom_state.rows = {
            {"外部定位源 | external_source"},
            {"外部里程计输入话题 | subtopic_name_external_odom"},
            {"里程计是否有效 | odometry_valid"},
            {"里程计更新频率 | odometry_update_hz"},
            {"重定位输入话题 | subtopic_name_external_relocalization"},
            {"发布的局部里程计话题 | pubtopic_name_local_odom"},
            {"发布的全局里程计话题 | pubtopic_name_global_odom"},
            {"世界坐标系 | world_frame_name"},
            {"全局坐标系 | global_frame_name"},
            {"局部坐标系 | local_frame_name"},
            {"机体坐标系 | base_frame_name"},
        };
        odom_state.uncovered_fields = {
            "local_odom",
            "global_odom",
            "world_to_global_tf",
            "global_to_local_tf",
            "local_to_base_tf"};

        TopicState control_state;
        control_state.key = "uav_control_state";
        control_state.title = "uav_control_state";
        control_state.ros_topic = "/uav1/sunray/uav_control/control_state";
        control_state.yunlink_name = "UavControlStateSnapshot";
        control_state.rows = {
            {"控制器类型 | controller_types"},
            {"起飞相对高度 | takeoff_relative_height_m"},
            {"起飞最大速度 | takeoff_max_velocity_mps"},
            {"降落类型 | land_type"},
            {"降落最大速度 | land_max_velocity_mps"},
            {"Home 点 X | home_point_m.x"},
            {"Home 点 Y | home_point_m.y"},
            {"Home 点 Z | home_point_m.z"},
            {"控制状态 | control_state"},
            {"最近控制命令 | last_control_cmd"},
            {"最近命令来源 | last_cmd_source"},
            {"里程计丢失 | odometry_lost"},
            {"里程计有效 | odometry_valid"},
            {"自机高度 Z | self_odom_z_m"},
        };
        control_state.uncovered_fields = {
            "agent_name",
            "agent_id",
            "self_odom（除 z 以外的完整 pose/twist）",
            "controller_output_type",
            "position_target",
            "attitude_target",
        };

        TopicState mavros_state;
        mavros_state.key = "mavros_state";
        mavros_state.title = "mavros_state";
        mavros_state.ros_topic = "/uav1/mavros/state";
        mavros_state.yunlink_name = "MavrosStateSnapshot";
        mavros_state.rows = {
            {"飞控连接状态 | connected"},
            {"解锁状态 | armed"},
            {"引导模式可用 | guided"},
            {"模式名称 | mode_name"},
            {"系统状态 | system_status"},
        };

        TopicState px4_state;
        px4_state.key = "px4_state";
        px4_state.title = "px4_state";
        px4_state.ros_topic = "/uav1/sunray/px4_state";
        px4_state.yunlink_name = "Px4StateSnapshot";
        px4_state.rows = {
            {"飞控连接状态 | connected"},
            {"遥控链路可用 | rc_available"},
            {"解锁状态 | armed"},
            {"飞行模式编号 | flight_mode"},
            {"飞行模式名称 | flight_mode_name"},
            {"系统状态 | system_status"},
            {"落地状态 | landed_state"},
            {"电池电压 | battery_voltage_v"},
            {"电池电流 | battery_current_a"},
            {"电池百分比 | battery_percentage"},
            {"局部位置 X | local_position_m.x"},
            {"局部位置 Y | local_position_m.y"},
            {"局部位置 Z | local_position_m.z"},
            {"局部速度 X | local_velocity_mps.x"},
            {"局部速度 Y | local_velocity_mps.y"},
            {"局部速度 Z | local_velocity_mps.z"},
            {"偏航设定值 | yaw_setpoint_rad"},
            {"偏航角速度设定值 | yaw_rate_setpoint_radps"},
            {"卫星数量 | satellites"},
            {"GPS 状态 | gps_status"},
            {"GPS 服务 | gps_service"},
            {"纬度 | latitude_deg"},
            {"经度 | longitude_deg"},
            {"高度 | altitude_m"},
        };
        px4_state.uncovered_fields = {
            "header",
            "fcu_load",
            "external_pose",
            "external_velocity",
            "local_velocity.angular",
            "setpoint_coordinate_frame",
            "setpoint_local_type_mask",
            "pos_setpoint",
            "vel_setpoint",
            "acc_setpoint",
            "setpoint_att_type_mask",
            "orientation_setpoint",
            "body_rate_setpoint",
            "thrust_setpoint",
            "latitude_raw",
            "longitude_raw",
            "altitude_amsl",
        };

        topics_[local_odom.key] = local_odom;
        topics_[odom_state.key] = odom_state;
        topics_[control_state.key] = control_state;
        topics_[mavros_state.key] = mavros_state;
        topics_[px4_state.key] = px4_state;
    }

    void load_params() {
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

    void start_runtime() {
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

    void bind_yunlink_subscribers() {
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

    void bind_ros_subscribers() {
        local_odom_sub_ = nh_.subscribe(topics_["local_odom"].ros_topic,
                                        20,
                                        &CompareBackend::on_local_odom,
                                        this);
        odom_state_sub_ = nh_.subscribe(topics_["odom_state"].ros_topic,
                                        20,
                                        &CompareBackend::on_odom_state,
                                        this);
        control_state_sub_ = nh_.subscribe(topics_["uav_control_state"].ros_topic,
                                           20,
                                           &CompareBackend::on_control_state,
                                           this);
        mavros_state_sub_ = nh_.subscribe(topics_["mavros_state"].ros_topic,
                                          20,
                                          &CompareBackend::on_mavros_state,
                                          this);
        px4_state_sub_ =
            nh_.subscribe(topics_["px4_state"].ros_topic, 20, &CompareBackend::on_px4_state, this);
        log("ROS 原始话题订阅器已就绪");
    }

    void setup_reconnect_timer() {
        reconnect_timer_ =
            nh_.createTimer(ros::Duration(1.0), &CompareBackend::on_reconnect_timer, this);
    }

    static uint16_t clamp_port(int value) {
        if (value < 0) {
            return 0;
        }
        if (value > 65535) {
            return 65535;
        }
        return static_cast<uint16_t>(value);
    }

    void on_reconnect_timer(const ros::TimerEvent&) {
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

    void on_local_odom(const nav_msgs::Odometry::ConstPtr& msg) {
        std::unordered_map<std::string, std::string> values;
        fill_local_odom_from_ros(*msg, values);
        update_ros("local_odom", std::move(values), msg->header.stamp);
    }

    void on_odom_state(const sunray_msgs::OdomState::ConstPtr& msg) {
        std::unordered_map<std::string, std::string> values;
        fill_odom_state_from_ros(*msg, values);
        update_ros("odom_state", std::move(values), msg->header.stamp);
    }

    void on_control_state(const sunray_msgs::UAVControlState::ConstPtr& msg) {
        std::unordered_map<std::string, std::string> values;
        fill_control_state_from_ros(*msg, values);
        update_ros("uav_control_state", std::move(values), msg->header.stamp);
    }

    void on_mavros_state(const mavros_msgs::State::ConstPtr& msg) {
        std::unordered_map<std::string, std::string> values;
        fill_mavros_state_from_ros(*msg, values);
        update_ros("mavros_state", std::move(values), ros::Time::now());
    }

    void on_px4_state(const sunray_msgs::Px4State::ConstPtr& msg) {
        std::unordered_map<std::string, std::string> values;
        fill_px4_state_from_ros(*msg, values);
        update_ros("px4_state", std::move(values), msg->header.stamp);
    }

    void update_ros(const std::string& key,
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

    void update_yunlink(const std::string& key,
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

    void log(const std::string& line) {
        std::lock_guard<std::mutex> lock(mu_);
        logs_.push_back(line);
        if (logs_.size() > 16) {
            logs_.erase(logs_.begin());
        }
    }

    void log_throttle(const std::string& line) {
        const ros::Time now = ros::Time::now();
        if ((now - last_log_time_).toSec() < 2.0) {
            return;
        }
        last_log_time_ = now;
        log(line);
    }

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

class MainWindow : public QMainWindow {
  public:
    explicit MainWindow(CompareBackend* backend, QWidget* parent = nullptr)
        : QMainWindow(parent), backend_(backend), align_window_ms_(backend->align_window_ms()) {
        setWindowTitle("Sunray 与 Yunlink 数据一致性对比工具");
        resize(1880, 1020);
        build_ui();

        auto* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &MainWindow::refresh_view);
        timer->start(250);
    }

  private:
    void build_ui() {
        auto* central = new QWidget(this);
        auto* root_layout = new QVBoxLayout(central);
        root_layout->setContentsMargins(14, 14, 14, 14);
        root_layout->setSpacing(10);

        auto* title = new QLabel("Sunray 与 Yunlink 数据一致性对比工具", central);
        QFont title_font("DejaVu Sans", 18, QFont::Bold);
        title->setFont(title_font);
        title->setStyleSheet("color:#173127;");
        root_layout->addWidget(title);

        summary_label_ = new QLabel(central);
        summary_label_->setWordWrap(true);
        summary_label_->setStyleSheet(
            "background:#eff6ef;color:#284033;border:1px solid #b6cdb8;border-radius:10px;padding:10px;");
        root_layout->addWidget(summary_label_);

        auto* tabs = new QTabWidget(central);
        tabs->setStyleSheet(
            "QTabBar::tab { background:#d7e6da; padding:8px 14px; }"
            "QTabBar::tab:selected { background:#f2f6f0; }");
        root_layout->addWidget(tabs, 1);

        const std::vector<std::string> topic_order = {
            "local_odom", "odom_state", "uav_control_state", "mavros_state", "px4_state"};
        for (const auto& key : topic_order) {
            auto* page = new QWidget(tabs);
            auto* layout = new QVBoxLayout(page);
            layout->setContentsMargins(10, 10, 10, 10);
            layout->setSpacing(8);

            auto* info = new QLabel(page);
            info->setWordWrap(true);
            info->setStyleSheet(
                "background:#f7faf6;color:#304137;border:1px solid #c3d3c5;border-radius:8px;padding:8px;");
            topic_info_[key] = info;
            layout->addWidget(info);

            auto* compare_tabs = new QTabWidget(page);
            compare_tabs->setStyleSheet(
                "QTabBar::tab { background:#e5efe5; padding:6px 12px; }"
                "QTabBar::tab:selected { background:#fbfdfb; }");

            auto* latest_page = new QWidget(compare_tabs);
            auto* latest_layout = new QVBoxLayout(latest_page);
            latest_layout->setContentsMargins(0, 0, 0, 0);
            auto* latest_table = create_compare_table(
                latest_page, {"字段说明", "ROS 最新值", "Yunlink 最新值", "差值", "比对结果"});
            topic_latest_tables_[key] = latest_table;
            latest_layout->addWidget(latest_table);
            compare_tabs->addTab(latest_page, "最新值直比");

            auto* aligned_page = new QWidget(compare_tabs);
            auto* aligned_layout = new QVBoxLayout(aligned_page);
            aligned_layout->setContentsMargins(0, 0, 0, 0);
            auto* aligned_table = create_compare_table(
                aligned_page, {"字段说明", "ROS 对齐值", "Yunlink 对齐值", "差值", "比对结果"});
            topic_aligned_tables_[key] = aligned_table;
            aligned_layout->addWidget(aligned_table);
            compare_tabs->addTab(aligned_page, "时间对齐比");

            layout->addWidget(compare_tabs, 1);

            auto* uncovered = new QLabel(page);
            uncovered->setWordWrap(true);
            uncovered->setStyleSheet("color:#5a5f48;");
            uncovered_labels_[key] = uncovered;
            layout->addWidget(uncovered);

            tabs->addTab(page, QString::fromStdString(backend_->snapshot_topics().at(key).title));
        }

        logs_ = new QTextEdit(central);
        logs_->setReadOnly(true);
        logs_->setMinimumHeight(140);
        logs_->setStyleSheet(
            "QTextEdit { background:#13211b;color:#cde9d1;border-radius:8px;padding:6px;"
            "font-family:'DejaVu Sans Mono'; }");
        root_layout->addWidget(logs_);

        setCentralWidget(central);
    }

    void refresh_view() {
        const auto topics = backend_->snapshot_topics();
        const auto logs = backend_->snapshot_logs();

        std::ostringstream summary;
        summary << "范围：local_odom、odom_state、uav_control_state、mavros_state、px4_state\n";
        summary << "模式：最新值直比 | 时间对齐比（窗口 <= " << fmt_ms(align_window_ms_) << " ms）";
        summary_label_->setText(QString::fromStdString(summary.str()));

        for (const auto& item : topics) {
            const auto& key = item.first;
            const auto& topic = item.second;
            auto* info = topic_info_[key];
            auto* uncovered = uncovered_labels_[key];
            auto* latest_table = topic_latest_tables_[key];
            auto* aligned_table = topic_aligned_tables_[key];
            const ComparisonSelection latest_selection = make_latest_selection(topic);
            const ComparisonSelection aligned_selection = make_aligned_selection(topic, align_window_ms_);

            std::ostringstream info_ss;
            info_ss << "ROS: " << topic.ros_topic << "\n";
            info_ss << "Yunlink: " << topic.yunlink_name << "\n";
            info_ss << "ROS stamp " << fmt_ros_time(topic.ros.msg_stamp) << " | ROS rx "
                    << fmt_ros_time(topic.ros.receive_time) << "\n";
            info_ss << "Yunlink rx " << fmt_ros_time(topic.yunlink.receive_time);
            if (topic.yunlink.message_id != 0) {
                info_ss << " | msg_id " << topic.yunlink.message_id;
            }
            if (!topic.yunlink.note.empty()) {
                info_ss << " | " << topic.yunlink.note;
            }
            if (latest_selection.matched) {
                info_ss << "\n最新 dt " << fmt_ms(latest_selection.receive_dt_ms) << " ms";
            } else {
                info_ss << "\n最新 dt --";
            }
            if (aligned_selection.matched) {
                info_ss << " | 对齐 dt " << fmt_ms(aligned_selection.receive_dt_ms) << " ms";
            } else if (has_snapshot(aligned_selection.yunlink)) {
                info_ss << " | 对齐 dt > " << fmt_ms(align_window_ms_) << " ms";
            } else {
                info_ss << " | 对齐 dt --";
            }
            info->setText(QString::fromStdString(info_ss.str()));

            populate_compare_table(latest_table, topic, latest_selection);
            populate_compare_table(aligned_table, topic, aligned_selection);

            std::ostringstream uncovered_ss;
            uncovered_ss << "未覆盖字段：";
            for (size_t i = 0; i < topic.uncovered_fields.size(); ++i) {
                if (i > 0) {
                    uncovered_ss << ", ";
                }
                uncovered_ss << topic.uncovered_fields[i];
            }
            uncovered->setText(QString::fromStdString(uncovered_ss.str()));
        }

        std::ostringstream log_ss;
        for (const auto& line : logs) {
            log_ss << line << "\n";
        }
        logs_->setPlainText(QString::fromStdString(log_ss.str()));
    }

    static QTableWidget* create_compare_table(QWidget* parent, const QStringList& headers) {
        auto* table = new QTableWidget(parent);
        table->setColumnCount(headers.size());
        table->setHorizontalHeaderLabels(headers);
        table->horizontalHeader()->setStretchLastSection(false);
        table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
        table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
        table->verticalHeader()->setVisible(false);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::NoSelection);
        table->setAlternatingRowColors(true);
        table->setWordWrap(false);
        table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
        table->setStyleSheet(
            "QTableWidget { background:#fbfdfb; alternate-background-color:#f1f6f1; }");
        return table;
    }

    static void populate_compare_table(QTableWidget* table,
                                       const TopicState& topic,
                                       const ComparisonSelection& selection) {
        table->setRowCount(static_cast<int>(topic.rows.size()));
        for (int row = 0; row < static_cast<int>(topic.rows.size()); ++row) {
            const auto& template_row = topic.rows[row];
            const std::string key_name = row_key(topic.key, row);
            const auto ros_it = selection.ros.values.find(key_name);
            const auto yn_it = selection.yunlink.values.find(key_name);
            const std::string ros_value =
                ros_it == selection.ros.values.end() ? "--" : ros_it->second;
            const std::string yn_value =
                yn_it == selection.yunlink.values.end() ? "--" : yn_it->second;
            const bool has_both =
                ros_it != selection.ros.values.end() && yn_it != selection.yunlink.values.end();
            const bool numeric = has_both && is_numeric(ros_value) && is_numeric(yn_value);
            const bool equal =
                has_both &&
                (numeric ? equal_float(ros_value, yn_value, field_epsilon(topic.key, key_name))
                         : equal_text(ros_value, yn_value));
            const std::string delta = numeric ? delta_float(ros_value, yn_value) : "--";
            const std::string match = has_both ? (equal ? "OK" : "DIFF") : "WAIT";

            set_item(table, row, 0, template_row.label);
            set_item(table, row, 1, ros_value);
            set_item(table, row, 2, yn_value);
            set_item(table, row, 3, delta);
            auto* match_item = set_item(table, row, 4, match);
            table->item(row, 0)->setToolTip(QString::fromStdString(template_row.label));
            table->item(row, 1)->setToolTip(QString::fromStdString(ros_value));
            table->item(row, 2)->setToolTip(QString::fromStdString(yn_value));
            if (match == "OK") {
                match_item->setBackground(QBrush(QColor("#d7f0d6")));
            } else if (match == "DIFF") {
                match_item->setBackground(QBrush(QColor("#f7d7d7")));
            } else {
                match_item->setBackground(QBrush(QColor("#efe8c8")));
            }
        }
    }

    static bool is_numeric(const std::string& value) {
        if (value.empty() || value == "--") {
            return false;
        }
        char* end = nullptr;
        std::strtod(value.c_str(), &end);
        return end != nullptr && *end == '\0';
    }

    static QTableWidgetItem* set_item(QTableWidget* table,
                                      int row,
                                      int col,
                                      const std::string& text) {
        auto* item = table->item(row, col);
        if (item == nullptr) {
            item = new QTableWidgetItem();
            table->setItem(row, col, item);
        }
        item->setText(QString::fromStdString(text));
        item->setTextAlignment((col == 3 || col == 4) ? Qt::AlignCenter : (Qt::AlignLeft | Qt::AlignVCenter));
        return item;
    }

    static std::string row_key(const std::string& topic_key, int row) {
        if (topic_key == "local_odom") {
            static const std::vector<std::string> keys = {
                "position_m.x",       "position_m.y",       "position_m.z",       "orientation_x",
                "orientation_y",      "orientation_z",      "orientation_w",      "linear_velocity_mps.x",
                "linear_velocity_mps.y", "linear_velocity_mps.z"};
            return keys[row];
        }
        if (topic_key == "odom_state") {
            static const std::vector<std::string> keys = {
                "external_source",
                "subtopic_name_external_odom",
                "odometry_valid",
                "odometry_update_hz",
                "subtopic_name_external_relocalization",
                "pubtopic_name_local_odom",
                "pubtopic_name_global_odom",
                "world_frame_name",
                "global_frame_name",
                "local_frame_name",
                "base_frame_name"};
            return keys[row];
        }
        if (topic_key == "uav_control_state") {
            static const std::vector<std::string> keys = {
                "controller_types",
                "takeoff_relative_height_m",
                "takeoff_max_velocity_mps",
                "land_type",
                "land_max_velocity_mps",
                "home_point_m.x",
                "home_point_m.y",
                "home_point_m.z",
                "control_state",
                "last_control_cmd",
                "last_cmd_source",
                "odometry_lost",
                "odometry_valid",
                "self_odom_z_m"};
            return keys[row];
        }
        if (topic_key == "mavros_state") {
            static const std::vector<std::string> keys = {
                "connected", "armed", "guided", "mode_name", "system_status"};
            return keys[row];
        }
        static const std::vector<std::string> keys = {
            "connected",             "rc_available",           "armed",               "flight_mode",
            "flight_mode_name",      "system_status",          "landed_state",        "battery_voltage_v",
            "battery_current_a",     "battery_percentage",     "local_position_m.x",  "local_position_m.y",
            "local_position_m.z",    "local_velocity_mps.x",   "local_velocity_mps.y","local_velocity_mps.z",
            "yaw_setpoint_rad",      "yaw_rate_setpoint_radps","satellites",          "gps_status",
            "gps_service",           "latitude_deg",           "longitude_deg",       "altitude_m"};
        return keys[row];
    }

    CompareBackend* backend_;
    double align_window_ms_{kDefaultAlignWindowMs};
    QLabel* summary_label_{nullptr};
    QTextEdit* logs_{nullptr};
    std::unordered_map<std::string, QLabel*> topic_info_;
    std::unordered_map<std::string, QLabel*> uncovered_labels_;
    std::unordered_map<std::string, QTableWidget*> topic_latest_tables_;
    std::unordered_map<std::string, QTableWidget*> topic_aligned_tables_;
};

}  // namespace

int main(int argc, char** argv) {
    ros::init(argc, argv, "sunray_yunlink_compare_ui");
    QApplication app(argc, argv);

    CompareBackend backend;
    MainWindow window(&backend);
    window.show();

    QTimer ros_timer;
    QObject::connect(&ros_timer, &QTimer::timeout, []() { ros::spinOnce(); });
    ros_timer.start(20);

    return app.exec();
}
