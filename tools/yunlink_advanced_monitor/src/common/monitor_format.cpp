#include "common/monitor_format.hpp"

#include <cmath>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

double monitor_rad_to_deg(double radians) {
    constexpr double kPi = 3.14159265358979323846;
    return radians * (180.0 / kPi);
}

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

std::string monitor_fmt_degrees(double radians) {
    return monitor_fmt_float(monitor_rad_to_deg(radians));
}

std::string monitor_fmt_degrees_per_sec(double radians_per_sec) {
    return monitor_fmt_float(monitor_rad_to_deg(radians_per_sec));
}

std::string monitor_fmt_bool(bool value) {
    return value ? "true" : "false";
}

std::string monitor_fmt_timestamp_ns(uint64_t stamp_ns) {
    if (stamp_ns == 0) {
        return "-";
    }
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(3);
    ss << (static_cast<double>(stamp_ns) / 1000000000.0);
    return ss.str() + " s";
}

std::string monitor_fmt_age_ms(uint64_t age_ms) {
    return std::to_string(age_ms) + " ms";
}

std::string monitor_fmt_percent(double value) {
    std::ostringstream ss;
    ss.setf(std::ios::fixed);
    ss.precision(1);
    ss << (value * 100.0);
    return ss.str() + "%";
}

bool monitor_parse_bool(const std::string& value, bool* out) {
    if (value == "true") {
        if (out != nullptr) {
            *out = true;
        }
        return true;
    }
    if (value == "false") {
        if (out != nullptr) {
            *out = false;
        }
        return true;
    }
    return false;
}

bool monitor_parse_u64(const std::string& value, uint64_t* out) {
    try {
        const auto parsed = std::stoull(value);
        if (out != nullptr) {
            *out = static_cast<uint64_t>(parsed);
        }
        return true;
    } catch (...) {
        return false;
    }
}

bool monitor_parse_u32(const std::string& value, uint32_t* out) {
    uint64_t parsed = 0;
    if (!monitor_parse_u64(value, &parsed) || parsed > static_cast<uint64_t>(UINT32_MAX)) {
        return false;
    }
    if (out != nullptr) {
        *out = static_cast<uint32_t>(parsed);
    }
    return true;
}

bool monitor_parse_u8(const std::string& value, uint8_t* out) {
    uint64_t parsed = 0;
    if (!monitor_parse_u64(value, &parsed) || parsed > static_cast<uint64_t>(UINT8_MAX)) {
        return false;
    }
    if (out != nullptr) {
        *out = static_cast<uint8_t>(parsed);
    }
    return true;
}

bool monitor_parse_double(const std::string& value, double* out) {
    try {
        const auto parsed = std::stod(value);
        if (out != nullptr) {
            *out = parsed;
        }
        return true;
    } catch (...) {
        return false;
    }
}
