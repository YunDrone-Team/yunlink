#ifndef SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP

#include <string>
#include <unordered_map>

#include <geographic_msgs/GeoPoint.h>
#include <geometry_msgs/Pose.h>
#include <geometry_msgs/Quaternion.h>
#include <geometry_msgs/TransformStamped.h>
#include <geometry_msgs/Twist.h>
#include <geometry_msgs/Vector3.h>
#include <mavros_msgs/AttitudeTarget.h>
#include <mavros_msgs/PositionTarget.h>
#include <nav_msgs/Odometry.h>
#include <std_msgs/Header.h>
#include <sunray_msgs/UAVControlCMD.h>
#include <sunray_msgs/Vector2.h>
#include <yunlink/yunlink.hpp>

#include "model/format.hpp"

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value);
void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value);
void set_header(std::unordered_map<std::string, std::string>& values,
                const std::string& prefix,
                const std_msgs::Header& msg);
void set_header(std::unordered_map<std::string, std::string>& values,
                const std::string& prefix,
                const yunlink::HeaderSnapshot& msg);
void set_vec2(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const sunray_msgs::Vector2& msg);
void set_vec2(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector2f& msg);
void set_vec3(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const geometry_msgs::Vector3& msg);
void set_vec3(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const geometry_msgs::Point& msg);
void set_vec3(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Vector3f& msg);
void set_quat(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const geometry_msgs::Quaternion& msg);
void set_quat(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::Quaternionf& msg);
void set_geo(std::unordered_map<std::string, std::string>& values,
             const std::string& prefix,
             const geographic_msgs::GeoPoint& msg);
void set_geo(std::unordered_map<std::string, std::string>& values,
             const std::string& prefix,
             const yunlink::GeoPointSnapshot& msg);
void set_pose(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const geometry_msgs::Pose& msg);
void set_pose(std::unordered_map<std::string, std::string>& values,
              const std::string& prefix,
              const yunlink::PoseSnapshot& msg);
void set_twist(std::unordered_map<std::string, std::string>& values,
               const std::string& prefix,
               const geometry_msgs::Twist& msg);
void set_twist(std::unordered_map<std::string, std::string>& values,
               const std::string& prefix,
               const yunlink::TwistSnapshot& msg);
void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const nav_msgs::Odometry& msg);
void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::LocalOdomSnapshot& msg);
void set_odometry(std::unordered_map<std::string, std::string>& values,
                  const std::string& prefix,
                  const yunlink::OdometrySnapshot& msg);
void set_transform(std::unordered_map<std::string, std::string>& values,
                   const std::string& prefix,
                   const geometry_msgs::TransformStamped& msg);
void set_transform(std::unordered_map<std::string, std::string>& values,
                   const std::string& prefix,
                   const yunlink::TransformSnapshot& msg);
void set_control_cmd(std::unordered_map<std::string, std::string>& values,
                     const std::string& prefix,
                     const sunray_msgs::UAVControlCMD& msg);
void set_control_cmd(std::unordered_map<std::string, std::string>& values,
                     const std::string& prefix,
                     const yunlink::UavControlCmdSnapshot& msg);
void set_position_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const mavros_msgs::PositionTarget& msg);
void set_position_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::PositionTargetSnapshot& msg);
void set_attitude_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const mavros_msgs::AttitudeTarget& msg);
void set_attitude_target(std::unordered_map<std::string, std::string>& values,
                         const std::string& prefix,
                         const yunlink::AttitudeTargetSnapshot& msg);

template <typename T>
void set_numeric(std::unordered_map<std::string, std::string>& values,
                 const std::string& key,
                 T value) {
    values[key] = fmt_num(value);
}

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP
