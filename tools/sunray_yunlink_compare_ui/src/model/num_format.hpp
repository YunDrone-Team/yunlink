#ifndef SUNRAY_YUNLINK_COMPARE_UI_MODEL_NUM_FORMAT_HPP
#define SUNRAY_YUNLINK_COMPARE_UI_MODEL_NUM_FORMAT_HPP

#include <cstdint>
#include <sstream>
#include <string>
#include <type_traits>

template <typename T>
std::string fmt_num(T value) {
    std::ostringstream ss;
    if constexpr (std::is_same_v<T, char> || std::is_same_v<T, signed char> ||
                  std::is_same_v<T, unsigned char> || std::is_same_v<T, std::int8_t> ||
                  std::is_same_v<T, std::uint8_t>) {
        ss << static_cast<int>(value);
    } else {
        ss << value;
    }
    return ss.str();
}

#endif  // SUNRAY_YUNLINK_COMPARE_UI_MODEL_NUM_FORMAT_HPP
