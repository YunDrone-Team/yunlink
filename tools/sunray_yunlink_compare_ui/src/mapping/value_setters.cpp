#include "mapping/value_setters.hpp"

void set_value(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               const std::string& value) {
    values[key] = value;
}

void set_float(std::unordered_map<std::string, std::string>& values,
               const std::string& key,
               double value) {
    values[key] = fmt_float(value);
}
