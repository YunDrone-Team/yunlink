#include "common/monitor_format.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

std::string monitor_fmt_float(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(6);
    ss << value;
    return ss.str();
}

std::string monitor_fmt_bool(bool value) {
    return value ? "true" : "false";
}

std::string monitor_fmt_ros_time(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << stamp.toSec();
    return ss.str();
}

std::string monitor_fmt_age(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    const double age = std::max(0.0, (ros::Time::now() - stamp).toSec());
    return monitor_fmt_float(age);
}
