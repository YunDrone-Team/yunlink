#include "common/monitor_format.hpp"

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
