#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP

#include <string>
#include <sstream>

#include <ros/ros.h>

std::string fmt_float(double value);
std::string fmt_bool(bool value);
std::string fmt_ros_time(const ros::Time& stamp);
std::string fmt_age(const ros::Time& stamp);
std::string fmt_ms(double value_ms);

template <typename T>
std::string fmt_num(T value) {
    std::ostringstream ss;
    ss << value;
    return ss.str();
}

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_FORMAT_HPP
