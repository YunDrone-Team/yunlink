#include <cstdint>
#include <iostream>
#include <string>

#include "tools/sunray_yunlink_compare_ui/src/model/num_format.hpp"

namespace {

template <typename T> bool expect_equal(T value, const std::string& expected, const char* label) {
    const std::string actual = fmt_num(value);
    if (actual != expected) {
        std::cerr << label << " expected [" << expected << "] but got [" << actual << "]\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    if (!expect_equal<std::uint8_t>(5, "5", "uint8_t")) {
        return 1;
    }
    if (!expect_equal<std::int8_t>(-3, "-3", "int8_t")) {
        return 1;
    }
    if (!expect_equal<unsigned char>(7, "7", "unsigned char")) {
        return 1;
    }
    if (!expect_equal<signed char>(-8, "-8", "signed char")) {
        return 1;
    }
    if (!expect_equal<char>('A', "65", "char")) {
        return 1;
    }
    if (!expect_equal<int>(42, "42", "int")) {
        return 1;
    }
    return 0;
}
