#include "mapping/value_setters.hpp"

#include "common/sunray_status_format.hpp"

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value) {
    values[key] = value;
}

void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value) {
    values[key] = monitor_fmt_float(value);
}

void set_header(std::unordered_map<std::string, std::string>& values,
                const std::string& prefix,
                const yunlink::HeaderSnapshot& msg) {
    set_value(values, prefix + "header.frame_id", msg.frame_id);
    set_numeric(values, prefix + "header.stamp_ns", msg.stamp_ns);
}

void set_vec2(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector2f& msg) {
    set_float(values, prefix + ".x", msg.x);
    set_float(values, prefix + ".y", msg.y);
}

void set_vec3(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector3f& msg) {
    set_float(values, prefix + ".x", msg.x);
    set_float(values, prefix + ".y", msg.y);
    set_float(values, prefix + ".z", msg.z);
}

void set_quat(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Quaternionf& msg) {
    set_float(values, prefix + ".x", msg.x);
    set_float(values, prefix + ".y", msg.y);
    set_float(values, prefix + ".z", msg.z);
    set_float(values, prefix + ".w", msg.w);
}

void set_geo(std::unordered_map<std::string, std::string>& values,
             const std::string& prefix,
             const yunlink::GeoPointSnapshot& msg) {
    set_float(values, prefix + "latitude_deg", msg.latitude_deg);
    set_float(values, prefix + "longitude_deg", msg.longitude_deg);
    set_float(values, prefix + "altitude_m", msg.altitude_m);
}

void set_pose(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::PoseSnapshot& msg) {
    set_vec3(values, prefix + "position_m", msg.position_m);
    set_quat(values, prefix + "orientation", msg.orientation);
}

void set_twist(std::unordered_map<std::string, std::string>& values,
               const std::string& prefix,
               const yunlink::TwistSnapshot& msg) {
    set_vec3(values, prefix + "linear_mps", msg.linear_mps);
    set_vec3(values, prefix + "angular_radps", msg.angular_radps);
}

void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::LocalOdomSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_value(values, prefix + "child_frame_id", msg.child_frame_id);
    set_pose(values, prefix + "pose.", msg.pose);
    set_twist(values, prefix + "twist.", msg.twist);
}

void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::OdometrySnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_value(values, prefix + "child_frame_id", msg.child_frame_id);
    set_pose(values, prefix + "pose.", msg.pose);
    set_twist(values, prefix + "twist.", msg.twist);
}

void set_transform(std::unordered_map<std::string, std::string>& values,
                   const std::string& prefix,
                   const yunlink::TransformSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_value(values, prefix + "child_frame_id", msg.child_frame_id);
    set_vec3(values, prefix + "translation_m", msg.translation_m);
    set_quat(values, prefix + "rotation", msg.rotation);
}

void set_control_cmd(std::unordered_map<std::string, std::string>& values,
                     const std::string& prefix,
                     const yunlink::UavControlCmdSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_numeric(values, prefix + "cmd_source", msg.cmd_source);
    set_numeric(values, prefix + "control_cmd", msg.control_cmd);
    set_vec3(values, prefix + "desired_pos_m", msg.desired_pos_m);
    set_vec3(values, prefix + "desired_vel_mps", msg.desired_vel_mps);
    set_vec3(values, prefix + "desired_acc_mps2", msg.desired_acc_mps2);
    set_vec3(values, prefix + "desired_jerk", msg.desired_jerk);
    set_vec2(values, prefix + "desired_body_xy_pos_m", msg.desired_body_xy_pos_m);
    set_vec2(values, prefix + "desired_body_xy_vel_mps", msg.desired_body_xy_vel_mps);
    set_float(values, prefix + "fixed_height_m", msg.fixed_height_m);
    set_geo(values, prefix + "desired_wgs84_pos.", msg.desired_wgs84_pos);
    set_numeric(values, prefix + "yaw_mode", msg.yaw_mode);
    set_float(values, prefix + "desired_yaw_rad", msg.desired_yaw_rad);
    set_float(values, prefix + "desired_yaw_rate_radps", msg.desired_yaw_rate_radps);
}

void set_position_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::PositionTargetSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_numeric(values, prefix + "coordinate_frame", msg.coordinate_frame);
    set_numeric(values, prefix + "type_mask", msg.type_mask);
    set_vec3(values, prefix + "position_m", msg.position_m);
    set_vec3(values, prefix + "velocity_mps", msg.velocity_mps);
    set_vec3(values, prefix + "acceleration_or_force", msg.acceleration_or_force);
    set_float(values, prefix + "yaw_rad", msg.yaw_rad);
    set_float(values, prefix + "yaw_rate_radps", msg.yaw_rate_radps);
}

void set_attitude_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::AttitudeTargetSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_numeric(values, prefix + "type_mask", msg.type_mask);
    set_quat(values, prefix + "orientation", msg.orientation);
    set_vec3(values, prefix + "body_rate_radps", msg.body_rate_radps);
    set_float(values, prefix + "thrust", msg.thrust);
}

void set_topic_diagnostic(std::unordered_map<std::string, std::string>& values,
                          const std::string& prefix,
                          const yunlink::SunrayTopicDiagnosticSnapshot& msg) {
    set_value(values, prefix + "key", msg.key);
    set_value(values, prefix + "topic", msg.topic);
    set_value(values, prefix + "configured", monitor_fmt_bool(msg.configured));
    set_value(values, prefix + "has_message", monitor_fmt_bool(msg.has_message));
    set_numeric(values, prefix + "publisher_count", msg.publisher_count);
    set_numeric(values, prefix + "message_count", msg.message_count);
    set_float(values, prefix + "hz", msg.hz);
    set_numeric(values, prefix + "age_ms", msg.age_ms);
    set_value(values, prefix + "stale", monitor_fmt_bool(msg.stale));
    set_value(values, prefix + "status", msg.status);
    set_value(values, prefix + "detail", msg.detail);
    set_value(values, prefix + "last_transition", msg.last_transition);
    set_numeric(values, prefix + "last_transition_age_ms", msg.last_transition_age_ms);
    set_numeric(values, prefix + "publish_fail_count", msg.publish_fail_count);
    set_float(values, prefix + "expected_min_hz", msg.expected_min_hz);
    set_value(values, prefix + "sparse", monitor_fmt_bool(msg.sparse));
}

void set_command_execution_status(std::unordered_map<std::string, std::string>& values,
                                  const std::string& prefix,
                                  const yunlink::CommandExecutionStatusSnapshot& msg) {
    set_header(values, prefix, msg.header);
    set_value(values, prefix + "agent_name", msg.agent_name);
    set_numeric(values, prefix + "agent_id", msg.agent_id);
    set_numeric(values, prefix + "session_id", msg.session_id);
    set_numeric(values, prefix + "command_message_id", msg.command_message_id);
    set_numeric(values, prefix + "command_correlation_id", msg.command_correlation_id);
    set_numeric(values, prefix + "command_kind", static_cast<uint16_t>(msg.command_kind));
    set_value(values,
              prefix + "command_kind_name",
              command_kind_name(static_cast<uint16_t>(msg.command_kind)));
    set_numeric(values, prefix + "execution_state", msg.execution_state);
    set_value(values,
              prefix + "execution_state_name",
              command_execution_state_name(msg.execution_state));
    set_numeric(values, prefix + "progress_percent", msg.progress_percent);
    set_value(values, prefix + "progress_percent_text", std::to_string(msg.progress_percent) + "%");
    set_value(values, prefix + "active", monitor_fmt_bool(msg.active));
    set_value(values, prefix + "terminal", monitor_fmt_bool(msg.terminal));
    set_value(values, prefix + "success", monitor_fmt_bool(msg.success));
    set_numeric(values, prefix + "result_code", msg.result_code);
    set_value(values, prefix + "detail", msg.detail);
    set_numeric(values, prefix + "control_state", msg.control_state);
    set_value(values, prefix + "control_state_name", uav_control_fsm_name(msg.control_state));
    set_numeric(values, prefix + "px4_landed_state", msg.px4_landed_state);
    set_value(values,
              prefix + "px4_landed_state_name",
              px4_landed_state_name(msg.px4_landed_state));
    set_value(values, prefix + "ready_for_takeoff", monitor_fmt_bool(msg.ready_for_takeoff));
    set_value(values, prefix + "ready_for_land", monitor_fmt_bool(msg.ready_for_land));
    set_value(values, prefix + "busy_reason", msg.busy_reason);
}
