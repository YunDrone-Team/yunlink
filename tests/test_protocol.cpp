/**
 * @file tests/test_protocol.cpp
 * @brief SunrayComLib source file.
 */

#include <iostream>

#include "sunraycom/core/protocol_codec.hpp"

int main() {
    sunraycom::ProtocolCodec codec;
    sunraycom::Frame f;
    f.header.seq = 201;
    f.header.robot_id = 7;
    f.payload = {1, 2, 3, 4, 5};

    const auto bytes = codec.encode(f, true);
    if (bytes.empty()) {
        std::cerr << "encode failed\n";
        return 1;
    }

    const auto dr = codec.decode(bytes.data(), bytes.size());
    if (!dr.ok()) {
        std::cerr << "decode failed\n";
        return 2;
    }

    if (dr.frame.header.seq != f.header.seq || dr.frame.header.robot_id != f.header.robot_id ||
        dr.frame.payload != f.payload) {
        std::cerr << "roundtrip mismatch\n";
        return 3;
    }

    return 0;
}
