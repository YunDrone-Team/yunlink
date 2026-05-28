#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

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
