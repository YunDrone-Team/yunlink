#include <cassert>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::map<std::string, std::string> vectors() {
    std::ifstream input(std::string(YUNLINK_SOURCE_DIR) +
                        "/profiles/org.yunlink.visual/v1/golden/visual-v1-vectors.txt");
    assert(input.good());
    std::map<std::string, std::string> result;
    for (std::string line; std::getline(input, line);) {
        if (line.empty() || line.front() == '#') {
            continue;
        }
        const auto separator = line.find('=');
        assert(separator != std::string::npos);
        result.emplace(line.substr(0, separator), line.substr(separator + 1));
    }
    return result;
}

std::vector<uint8_t> hex(const std::string& value) {
    assert(value.size() % 2 == 0);
    std::vector<uint8_t> bytes;
    bytes.reserve(value.size() / 2);
    const auto nibble = [](char character) {
        if (character >= '0' && character <= '9') {
            return static_cast<uint8_t>(character - '0');
        }
        return static_cast<uint8_t>(std::tolower(static_cast<unsigned char>(character)) - 'a' + 10);
    };
    for (size_t index = 0; index < value.size(); index += 2) {
        bytes.push_back(static_cast<uint8_t>((nibble(value[index]) << 4) | nibble(value[index + 1])));
    }
    return bytes;
}

uint16_t read_u16(const std::vector<uint8_t>& bytes, size_t offset) {
    return static_cast<uint16_t>(bytes.at(offset)) |
           static_cast<uint16_t>(bytes.at(offset + 1) << 8);
}

uint32_t read_u32(const std::vector<uint8_t>& bytes, size_t offset) {
    uint32_t value = 0;
    for (size_t index = 0; index != 4; ++index) {
        value |= static_cast<uint32_t>(bytes.at(offset + index)) << (8 * index);
    }
    return value;
}

bool valid_ylpc(const std::vector<uint8_t>& bytes) {
    return bytes.size() >= 16 && std::string(bytes.begin(), bytes.begin() + 4) == "YLPC" &&
           read_u16(bytes, 4) == 1 && (read_u16(bytes, 6) & ~uint16_t{1}) == 0 &&
           read_u32(bytes, 12) == 16 && bytes.size() == 16U + read_u32(bytes, 8) * 16U;
}

}  // namespace

int main() {
    const auto values = vectors();
    assert(valid_ylpc(hex(values.at("point_cloud.valid.hex"))));
    assert(!valid_ylpc(hex(values.at("point_cloud.invalid_stride.hex"))));
    assert(!valid_ylpc(hex(values.at("point_cloud.invalid_truncated.hex"))));
    assert(values.at("image.raw.hex").size() == 12);

    const auto& add = values.at("marker.add.json");
    assert(add.find("\"reference_frame\"") != std::string::npos);
    assert(add.find("\"lifetime_ns\"") != std::string::npos);
    assert(add.find("\"frame_locked\"") != std::string::npos);
    assert(values.at("marker.delete.json").find("\"action\":2") != std::string::npos);
    assert(values.at("marker.invalid_quaternion.json").find("[0,0,0,0]") != std::string::npos);
    return 0;
}
