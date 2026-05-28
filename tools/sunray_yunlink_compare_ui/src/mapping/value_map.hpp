#ifndef SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_MAP_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_MAP_HPP

#include <string>
#include <unordered_map>

#include <mavros_msgs/State.h>
#include <nav_msgs/Odometry.h>
#include <sunray_msgs/OdomState.h>
#include <sunray_msgs/Px4State.h>
#include <sunray_msgs/UAVControlState.h>
#include <yunlink/yunlink.hpp>

void fill_local_odom_from_ros(const nav_msgs::Odometry& msg,
                              std::unordered_map<std::string, std::string>& values);
void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values);
void fill_odom_state_from_ros(const sunray_msgs::OdomState& msg,
                              std::unordered_map<std::string, std::string>& values);
void fill_odom_state_from_yunlink(const yunlink::OdomStateSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values);
void fill_control_state_from_ros(const sunray_msgs::UAVControlState& msg,
                                 std::unordered_map<std::string, std::string>& values);
void fill_control_state_from_yunlink(const yunlink::UavControlStateSnapshot& msg,
                                     std::unordered_map<std::string, std::string>& values);
void fill_mavros_state_from_ros(const mavros_msgs::State& msg,
                                std::unordered_map<std::string, std::string>& values);
void fill_mavros_state_from_yunlink(const yunlink::MavrosStateSnapshot& msg,
                                    std::unordered_map<std::string, std::string>& values);
void fill_px4_state_from_ros(const sunray_msgs::Px4State& msg,
                             std::unordered_map<std::string, std::string>& values);
void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values);

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_MAP_HPP
