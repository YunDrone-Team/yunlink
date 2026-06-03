#ifndef SUNRAY_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP
#define SUNRAY_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP

#include <string>

#include <ros/ros.h>

#include "common/monitor_num_format.hpp"

std::string monitor_fmt_float(double value);
std::string monitor_fmt_bool(bool value);
std::string monitor_fmt_ros_time(const ros::Time& stamp);
std::string monitor_fmt_age(const ros::Time& stamp);

#endif  // SUNRAY_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP
