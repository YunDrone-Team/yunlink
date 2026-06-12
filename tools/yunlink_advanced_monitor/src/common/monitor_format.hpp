#ifndef YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP
#define YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP

#include <cstdint>
#include <string>

#include "common/monitor_num_format.hpp"

std::string monitor_fmt_float(double value);
std::string monitor_fmt_degrees(double radians);
std::string monitor_fmt_degrees_per_sec(double radians_per_sec);
std::string monitor_fmt_bool(bool value);
std::string monitor_fmt_timestamp_ns(uint64_t stamp_ns);
std::string monitor_fmt_age_ms(uint64_t age_ms);
std::string monitor_fmt_percent(double value);
double monitor_rad_to_deg(double radians);
bool monitor_parse_bool(const std::string& value, bool* out);
bool monitor_parse_u64(const std::string& value, uint64_t* out);
bool monitor_parse_u32(const std::string& value, uint32_t* out);
bool monitor_parse_u8(const std::string& value, uint8_t* out);
bool monitor_parse_double(const std::string& value, double* out);

#endif  // YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_FORMAT_HPP
