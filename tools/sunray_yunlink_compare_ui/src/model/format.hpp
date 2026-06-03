#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP

#include <string>

#include <ros/ros.h>

#include "model/num_format.hpp"

std::string fmt_float(double value);
std::string fmt_bool(bool value);
std::string fmt_ros_time(const ros::Time& stamp);
std::string fmt_age(const ros::Time& stamp);
std::string fmt_ms(double value_ms);

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP
