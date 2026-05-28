#include "model/format.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

std::string fmt_float(double value) {
    if (std::isnan(value)) {
        return "nan";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(6);
    ss << value;
    return ss.str();
}

std::string fmt_bool(bool value) {
    return value ? "true" : "false";
}

std::string fmt_ros_time(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << stamp.toSec();
    return ss.str();
}

std::string fmt_age(const ros::Time& stamp) {
    if (stamp.isZero()) {
        return "--";
    }
    const double age = std::max(0.0, (ros::Time::now() - stamp).toSec());
    return fmt_float(age);
}

std::string fmt_ms(double value_ms) {
    if (std::isnan(value_ms)) {
        return "--";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << value_ms;
    return ss.str();
}
