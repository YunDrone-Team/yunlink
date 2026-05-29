#include "model/topic_defs.hpp"

#include <utility>

namespace {

FieldView field(std::string key, std::string label) {
    FieldView view;
    view.key = std::move(key);
    view.label = std::move(label);
    return view;
}

void append_header_rows(std::vector<FieldView>& rows, const std::string& prefix) {
    rows.push_back(field(prefix + "header.frame_id", "Header frame | " + prefix + "header.frame_id"));
}

void append_vec2_rows(std::vector<FieldView>& rows,
                      const std::string& prefix,
                      const std::string& label) {
    rows.push_back(field(prefix + ".x", label + " X | " + prefix + ".x"));
    rows.push_back(field(prefix + ".y", label + " Y | " + prefix + ".y"));
}

void append_vec3_rows(std::vector<FieldView>& rows,
                      const std::string& prefix,
                      const std::string& label) {
    rows.push_back(field(prefix + ".x", label + " X | " + prefix + ".x"));
    rows.push_back(field(prefix + ".y", label + " Y | " + prefix + ".y"));
    rows.push_back(field(prefix + ".z", label + " Z | " + prefix + ".z"));
}

void append_quat_rows(std::vector<FieldView>& rows,
                      const std::string& prefix,
                      const std::string& label) {
    rows.push_back(field(prefix + ".x", label + " X | " + prefix + ".x"));
    rows.push_back(field(prefix + ".y", label + " Y | " + prefix + ".y"));
    rows.push_back(field(prefix + ".z", label + " Z | " + prefix + ".z"));
    rows.push_back(field(prefix + ".w", label + " W | " + prefix + ".w"));
}

void append_pose_rows(std::vector<FieldView>& rows,
                      const std::string& prefix,
                      const std::string& label) {
    append_vec3_rows(rows, prefix + "position_m", label + "位置");
    append_quat_rows(rows, prefix + "orientation", label + "姿态四元数");
}

void append_twist_rows(std::vector<FieldView>& rows,
                       const std::string& prefix,
                       const std::string& label) {
    append_vec3_rows(rows, prefix + "linear_mps", label + "线速度");
    append_vec3_rows(rows, prefix + "angular_radps", label + "角速度");
}

void append_odometry_rows(std::vector<FieldView>& rows,
                          const std::string& prefix,
                          const std::string& label) {
    append_header_rows(rows, prefix);
    rows.push_back(field(prefix + "child_frame_id", label + " child_frame_id | " + prefix + "child_frame_id"));
    append_pose_rows(rows, prefix + "pose.", label);
    append_twist_rows(rows, prefix + "twist.", label);
}

void append_geo_rows(std::vector<FieldView>& rows,
                     const std::string& prefix,
                     const std::string& label) {
    rows.push_back(field(prefix + "latitude_deg", label + "纬度 | " + prefix + "latitude_deg"));
    rows.push_back(field(prefix + "longitude_deg", label + "经度 | " + prefix + "longitude_deg"));
    rows.push_back(field(prefix + "altitude_m", label + "高度 | " + prefix + "altitude_m"));
}

void append_control_cmd_rows(std::vector<FieldView>& rows,
                             const std::string& prefix,
                             const std::string& label) {
    append_header_rows(rows, prefix);
    rows.push_back(field(prefix + "cmd_source", label + "命令来源 | " + prefix + "cmd_source"));
    rows.push_back(field(prefix + "control_cmd", label + "控制命令 | " + prefix + "control_cmd"));
    append_vec3_rows(rows, prefix + "desired_pos_m", label + "期望位置");
    append_vec3_rows(rows, prefix + "desired_vel_mps", label + "期望速度");
    append_vec3_rows(rows, prefix + "desired_acc_mps2", label + "期望加速度");
    append_vec3_rows(rows, prefix + "desired_jerk", label + "期望 jerk");
    append_vec2_rows(rows, prefix + "desired_body_xy_pos_m", label + "机体系 XY 位置");
    append_vec2_rows(rows, prefix + "desired_body_xy_vel_mps", label + "机体系 XY 速度");
    rows.push_back(field(prefix + "fixed_height_m", label + "固定高度 | " + prefix + "fixed_height_m"));
    append_geo_rows(rows, prefix + "desired_wgs84_pos.", label + "WGS84 目标");
    rows.push_back(field(prefix + "yaw_mode", label + "偏航模式 | " + prefix + "yaw_mode"));
    rows.push_back(field(prefix + "desired_yaw_rad", label + "期望偏航角 | " + prefix + "desired_yaw_rad"));
    rows.push_back(
        field(prefix + "desired_yaw_rate_radps", label + "期望偏航角速度 | " + prefix + "desired_yaw_rate_radps"));
}

void append_position_target_rows(std::vector<FieldView>& rows,
                                 const std::string& prefix,
                                 const std::string& label) {
    append_header_rows(rows, prefix);
    rows.push_back(field(prefix + "coordinate_frame", label + "坐标系 | " + prefix + "coordinate_frame"));
    rows.push_back(field(prefix + "type_mask", label + "掩码 | " + prefix + "type_mask"));
    append_vec3_rows(rows, prefix + "position_m", label + "位置 setpoint");
    append_vec3_rows(rows, prefix + "velocity_mps", label + "速度 setpoint");
    append_vec3_rows(rows, prefix + "acceleration_or_force", label + "加速度/力 setpoint");
    rows.push_back(field(prefix + "yaw_rad", label + "偏航角 | " + prefix + "yaw_rad"));
    rows.push_back(field(prefix + "yaw_rate_radps", label + "偏航角速度 | " + prefix + "yaw_rate_radps"));
}

void append_attitude_target_rows(std::vector<FieldView>& rows,
                                 const std::string& prefix,
                                 const std::string& label) {
    append_header_rows(rows, prefix);
    rows.push_back(field(prefix + "type_mask", label + "掩码 | " + prefix + "type_mask"));
    append_quat_rows(rows, prefix + "orientation", label + "姿态四元数");
    append_vec3_rows(rows, prefix + "body_rate_radps", label + "机体系角速度");
    rows.push_back(field(prefix + "thrust", label + "推力 | " + prefix + "thrust"));
}

void append_transform_rows(std::vector<FieldView>& rows,
                           const std::string& prefix,
                           const std::string& label) {
    append_header_rows(rows, prefix);
    rows.push_back(field(prefix + "child_frame_id", label + " child_frame_id | " + prefix + "child_frame_id"));
    append_vec3_rows(rows, prefix + "translation_m", label + "平移");
    append_quat_rows(rows, prefix + "rotation", label + "旋转四元数");
}

TopicState make_local_odom_topic() {
    TopicState topic;
    topic.key = "local_odom";
    topic.title = "local_odom";
    topic.ros_topic = "/uav1/sunray/localization/local_odom";
    topic.yunlink_name = "LocalOdomSnapshot";
    append_odometry_rows(topic.rows, "", "");
    return topic;
}

TopicState make_odom_state_topic() {
    TopicState topic;
    topic.key = "odom_state";
    topic.title = "odom_state";
    topic.ros_topic = "/uav1/sunray/localization/odom_state";
    topic.yunlink_name = "OdomStateSnapshot";
    topic.rows = {
        field("header.frame_id", "Header frame | header.frame_id"),
        field("external_source", "外部定位源 | external_source"),
        field("subtopic_name_external_odom", "外部里程计输入话题 | subtopic_name_external_odom"),
        field("odometry_valid", "里程计是否有效 | odometry_valid"),
        field("odometry_update_hz", "里程计更新频率 | odometry_update_hz"),
        field("subtopic_name_external_relocalization", "重定位输入话题 | subtopic_name_external_relocalization"),
        field("pubtopic_name_local_odom", "发布的局部里程计话题 | pubtopic_name_local_odom"),
        field("pubtopic_name_global_odom", "发布的全局里程计话题 | pubtopic_name_global_odom"),
    };
    append_odometry_rows(topic.rows, "local_odom.", "local_odom ");
    append_odometry_rows(topic.rows, "global_odom.", "global_odom ");
    topic.rows.push_back(field("world_frame_name", "世界坐标系 | world_frame_name"));
    topic.rows.push_back(field("global_frame_name", "全局坐标系 | global_frame_name"));
    topic.rows.push_back(field("local_frame_name", "局部坐标系 | local_frame_name"));
    topic.rows.push_back(field("base_frame_name", "机体坐标系 | base_frame_name"));
    append_transform_rows(topic.rows, "world_to_global_tf.", "world_to_global_tf");
    append_transform_rows(topic.rows, "global_to_local_tf.", "global_to_local_tf");
    append_transform_rows(topic.rows, "local_to_base_tf.", "local_to_base_tf");
    return topic;
}

TopicState make_control_state_topic() {
    TopicState topic;
    topic.key = "uav_control_state";
    topic.title = "uav_control_state";
    topic.ros_topic = "/uav1/sunray/uav_control/control_state";
    topic.yunlink_name = "UavControlStateSnapshot";
    topic.rows = {
        field("header.frame_id", "Header frame | header.frame_id"),
        field("agent_name", "机器人名称 | agent_name"),
        field("agent_id", "机器人编号 | agent_id"),
        field("controller_types", "控制器类型 | controller_types"),
        field("takeoff_relative_height_m", "起飞相对高度 | takeoff_relative_height_m"),
        field("takeoff_max_velocity_mps", "起飞最大速度 | takeoff_max_velocity_mps"),
        field("land_type", "降落类型 | land_type"),
        field("land_max_velocity_mps", "降落最大速度 | land_max_velocity_mps"),
    };
    append_vec3_rows(topic.rows, "home_point_m", "Home 点");
    topic.rows.push_back(field("control_state", "控制状态 | control_state"));
    append_control_cmd_rows(topic.rows, "last_cmd.", "last_cmd ");
    append_odometry_rows(topic.rows, "self_odom.", "self_odom ");
    topic.rows.push_back(field("odometry_lost", "里程计丢失 | odometry_lost"));
    topic.rows.push_back(field("odometry_valid", "里程计有效 | odometry_valid"));
    topic.rows.push_back(field("controller_output_type", "控制输出类型 | controller_output_type"));
    append_position_target_rows(topic.rows, "position_target.", "position_target ");
    append_attitude_target_rows(topic.rows, "attitude_target.", "attitude_target ");
    return topic;
}

TopicState make_mavros_state_topic() {
    TopicState topic;
    topic.key = "mavros_state";
    topic.title = "mavros_state";
    topic.ros_topic = "/uav1/mavros/state";
    topic.yunlink_name = "MavrosStateSnapshot";
    topic.rows = {
        field("header.frame_id", "Header frame | header.frame_id"),
        field("connected", "飞控连接状态 | connected"),
        field("armed", "解锁状态 | armed"),
        field("guided", "引导模式可用 | guided"),
        field("manual_input", "手动输入 | manual_input"),
        field("mode", "模式名称 | mode"),
        field("system_status", "系统状态 | system_status"),
    };
    return topic;
}

TopicState make_px4_state_topic() {
    TopicState topic;
    topic.key = "px4_state";
    topic.title = "px4_state";
    topic.ros_topic = "/uav1/sunray/px4_state";
    topic.yunlink_name = "Px4StateSnapshot";
    topic.rows = {
        field("header.frame_id", "Header frame | header.frame_id"),
        field("connected", "飞控连接状态 | connected"),
        field("rc_available", "遥控链路可用 | rc_available"),
        field("armed", "解锁状态 | armed"),
        field("flight_mode", "飞行模式编号 | flight_mode"),
        field("system_status", "系统状态 | system_status"),
        field("landed_state", "落地状态 | landed_state"),
        field("battery_voltage_v", "电池电压 | battery_voltage_v"),
        field("battery_current_a", "电池电流 | battery_current_a"),
        field("battery_percentage", "电池百分比 | battery_percentage"),
        field("fcu_load", "FCU 负载 | fcu_load"),
    };
    append_pose_rows(topic.rows, "external_pose.", "外部里程计");
    append_twist_rows(topic.rows, "external_velocity.", "外部里程计");
    append_pose_rows(topic.rows, "local_pose.", "局部");
    append_twist_rows(topic.rows, "local_velocity.", "局部");
    topic.rows.push_back(field("setpoint_coordinate_frame", "setpoint 坐标系 | setpoint_coordinate_frame"));
    topic.rows.push_back(field("setpoint_local_type_mask", "local setpoint 掩码 | setpoint_local_type_mask"));
    append_vec3_rows(topic.rows, "pos_setpoint_m", "位置 setpoint");
    append_vec3_rows(topic.rows, "vel_setpoint_mps", "速度 setpoint");
    append_vec3_rows(topic.rows, "acc_setpoint_mps2", "加速度 setpoint");
    topic.rows.push_back(field("yaw_setpoint_rad", "偏航设定值 | yaw_setpoint_rad"));
    topic.rows.push_back(field("yaw_rate_setpoint_radps", "偏航角速度设定值 | yaw_rate_setpoint_radps"));
    topic.rows.push_back(field("setpoint_att_type_mask", "attitude setpoint 掩码 | setpoint_att_type_mask"));
    append_quat_rows(topic.rows, "orientation_setpoint", "姿态 setpoint");
    append_vec3_rows(topic.rows, "body_rate_setpoint_radps", "机体系角速度 setpoint");
    topic.rows.push_back(field("thrust_setpoint", "推力 setpoint | thrust_setpoint"));
    topic.rows.push_back(field("satellites", "卫星数量 | satellites"));
    topic.rows.push_back(field("gps_status", "GPS 状态 | gps_status"));
    topic.rows.push_back(field("gps_service", "GPS 服务 | gps_service"));
    topic.rows.push_back(field("latitude_deg", "纬度 | latitude_deg"));
    topic.rows.push_back(field("longitude_deg", "经度 | longitude_deg"));
    topic.rows.push_back(field("altitude_m", "高度 | altitude_m"));
    topic.rows.push_back(field("latitude_raw_deg", "原始纬度 | latitude_raw_deg"));
    topic.rows.push_back(field("longitude_raw_deg", "原始经度 | longitude_raw_deg"));
    topic.rows.push_back(field("altitude_amsl_m", "AMSL 高度 | altitude_amsl_m"));
    return topic;
}

}  // namespace

std::unordered_map<std::string, TopicState> make_default_topics() {
    std::unordered_map<std::string, TopicState> topics;
    for (auto topic : {make_local_odom_topic(),
                       make_odom_state_topic(),
                       make_control_state_topic(),
                       make_mavros_state_topic(),
                       make_px4_state_topic()}) {
        topics[topic.key] = std::move(topic);
    }
    return topics;
}

std::vector<std::string> topic_display_order() {
    return {"local_odom", "odom_state", "uav_control_state", "mavros_state", "px4_state"};
}
