#ifndef SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP

#include <string>
#include <unordered_map>

#include "model/format.hpp"

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value);
void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value);

template <typename T>
void set_numeric(std::unordered_map<std::string, std::string>& values,
                 const std::string& key,
                 T value) {
    values[key] = fmt_num(value);
}

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MAPPING_VALUE_SETTERS_HPP
