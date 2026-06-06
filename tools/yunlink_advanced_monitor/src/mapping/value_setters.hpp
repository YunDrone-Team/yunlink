#ifndef YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_SETTERS_HPP
#define YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_SETTERS_HPP

#include <string>
#include <unordered_map>

#include <yunlink/yunlink.hpp>

#include "common/monitor_format.hpp"

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value);
void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value);
void set_header(std::unordered_map<std::string, std::string>& values,
                const std::string& prefix,
                const yunlink::HeaderSnapshot& msg);
void set_vec2(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector2f& msg);
void set_vec3(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector3f& msg);
void set_quat(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Quaternionf& msg);
void set_geo(std::unordered_map<std::string, std::string>& values,
             const std::string& prefix,
             const yunlink::GeoPointSnapshot& msg);
void set_pose(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::PoseSnapshot& msg);
void set_twist(std::unordered_map<std::string, std::string>& values,
               const std::string& prefix,
               const yunlink::TwistSnapshot& msg);
void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::LocalOdomSnapshot& msg);
void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::OdometrySnapshot& msg);
void set_transform(std::unordered_map<std::string, std::string>& values,
                   const std::string& prefix,
                   const yunlink::TransformSnapshot& msg);
void set_control_cmd(std::unordered_map<std::string, std::string>& values,
                     const std::string& prefix,
                     const yunlink::UavControlCmdSnapshot& msg);
void set_position_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::PositionTargetSnapshot& msg);
void set_attitude_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::AttitudeTargetSnapshot& msg);

template <typename T>
void set_numeric(std::unordered_map<std::string, std::string>& values,
                 const std::string& key,
                 T value) {
    values[key] = monitor_fmt_num(value);
}

#endif  // YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_SETTERS_HPP
