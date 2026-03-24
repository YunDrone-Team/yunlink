/**
 * @file tests/test_parser.cpp
 * @brief SunrayComLib source file.
 */

#include <iostream>

#include "sunraycom/core/frame_stream_parser.hpp"

int main() {
    sunraycom::ProtocolCodec codec;
    sunraycom::Frame a;
    a.header.seq = 200;
    a.header.robot_id = 1;
    a.payload = {9, 8, 7};

    sunraycom::Frame b;
    b.header.seq = 201;
    b.header.robot_id = 2;
    b.payload = {6, 5, 4, 3};

    const auto ba = codec.encode(a, true);
    const auto bb = codec.encode(b, true);

    sunraycom::FrameStreamParser parser;
    parser.feed(ba.data(), 5);
    parser.feed(ba.data() + 5, ba.size() - 5);
    parser.feed(bb);

    sunraycom::Frame out;
    if (!parser.pop_next(&out) || out.header.seq != a.header.seq) {
        std::cerr << "first frame failed\n";
        return 1;
    }
    if (!parser.pop_next(&out) || out.header.seq != b.header.seq) {
        std::cerr << "second frame failed\n";
        return 2;
    }
    return 0;
}
