#include "model/topic_defs.hpp"

namespace {

TopicState make_local_odom_topic() {
    TopicState topic;
    topic.key = "local_odom";
    topic.title = "local_odom";
    topic.ros_topic = "/uav1/sunray/localization/local_odom";
    topic.yunlink_name = "LocalOdomSnapshot";
    topic.rows = {
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
    return topic;
}

TopicState make_odom_state_topic() {
    TopicState topic;
    topic.key = "odom_state";
    topic.title = "odom_state";
    topic.ros_topic = "/uav1/sunray/localization/odom_state";
    topic.yunlink_name = "OdomStateSnapshot";
    topic.rows = {
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
    topic.uncovered_fields = {
        "local_odom",
        "global_odom",
        "world_to_global_tf",
        "global_to_local_tf",
        "local_to_base_tf"};
    return topic;
}

TopicState make_control_state_topic() {
    TopicState topic;
    topic.key = "uav_control_state";
    topic.title = "uav_control_state";
    topic.ros_topic = "/uav1/sunray/uav_control/control_state";
    topic.yunlink_name = "UavControlStateSnapshot";
    topic.rows = {
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
    topic.uncovered_fields = {
        "agent_name",
        "agent_id",
        "self_odom（除 z 以外的完整 pose/twist）",
        "controller_output_type",
        "position_target",
        "attitude_target",
    };
    return topic;
}

TopicState make_mavros_state_topic() {
    TopicState topic;
    topic.key = "mavros_state";
    topic.title = "mavros_state";
    topic.ros_topic = "/uav1/mavros/state";
    topic.yunlink_name = "MavrosStateSnapshot";
    topic.rows = {
        {"飞控连接状态 | connected"},
        {"解锁状态 | armed"},
        {"引导模式可用 | guided"},
        {"模式名称 | mode_name"},
        {"系统状态 | system_status"},
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
    topic.uncovered_fields = {
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

std::string row_key(const std::string& topic_key, int row) {
    if (topic_key == "local_odom") {
        static const std::vector<std::string> keys = {
            "position_m.x",
            "position_m.y",
            "position_m.z",
            "orientation_x",
            "orientation_y",
            "orientation_z",
            "orientation_w",
            "linear_velocity_mps.x",
            "linear_velocity_mps.y",
            "linear_velocity_mps.z"};
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
        "connected",
        "rc_available",
        "armed",
        "flight_mode",
        "flight_mode_name",
        "system_status",
        "landed_state",
        "battery_voltage_v",
        "battery_current_a",
        "battery_percentage",
        "local_position_m.x",
        "local_position_m.y",
        "local_position_m.z",
        "local_velocity_mps.x",
        "local_velocity_mps.y",
        "local_velocity_mps.z",
        "yaw_setpoint_rad",
        "yaw_rate_setpoint_radps",
        "satellites",
        "gps_status",
        "gps_service",
        "latitude_deg",
        "longitude_deg",
        "altitude_m"};
    return keys[row];
}
