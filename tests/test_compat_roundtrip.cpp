/**
 * @file tests/test_compat_roundtrip.cpp
 * @brief SunrayComLib source file.
 */

#include <iostream>

#include "sunraycom/compat/legacy_adapter.hpp"

int main() {
    sunraycom::compat::LegacyAdapter adapter;

    ::DataFrame df{};
    df.seq = MessageID::PointCloudDataSwitchMessageID;
    df.robot_ID = 3;
    df.data.pointCloudDataSwitch.init();
    df.data.pointCloudDataSwitch.dataSwitch = true;

    const auto bytes = adapter.encode_legacy(df);
    if (bytes.empty()) {
        std::cerr << "legacy encode failed\n";
        return 1;
    }

    ::DataFrame decoded{};
    if (!adapter.decode_legacy(bytes, &decoded)) {
        std::cerr << "legacy decode failed\n";
        return 2;
    }

    if (decoded.seq != MessageID::PointCloudDataSwitchMessageID ||
        decoded.data.pointCloudDataSwitch.dataSwitch != true) {
        std::cerr << "point cloud switch decode mismatch\n";
        return 3;
    }

    auto core_frame = adapter.to_core_frame(df);
    if (!core_frame.has_value()) {
        std::cerr << "to_core_frame failed\n";
        return 4;
    }

    ::DataFrame back{};
    if (!adapter.from_core_frame(core_frame.value(), &back)) {
        std::cerr << "from_core_frame failed\n";
        return 5;
    }

    if (back.seq != df.seq) {
        std::cerr << "compat mapping mismatch\n";
        return 6;
    }

    return 0;
}
