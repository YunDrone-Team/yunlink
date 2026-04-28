/**
 * @file tests/core/fuzz_protocol_codec.cpp
 * @brief Opt-in local fuzz-style harness for ProtocolCodec decode paths.
 */

#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

#include "yunlink/core/protocol_codec.hpp"

int main(int argc, char** argv) {
    size_t iterations = 10000;
    if (argc > 1) {
        iterations = static_cast<size_t>(std::stoull(argv[1]));
    }

    std::mt19937_64 rng(0xC0DEC0DEULL);
    std::uniform_int_distribution<size_t> length_dist(0, 256);
    std::uniform_int_distribution<uint32_t> byte_dist(0, 255);

    yunlink::ProtocolCodec codec;
    std::vector<uint8_t> bytes;

    for (size_t i = 0; i < iterations; ++i) {
        const size_t len = length_dist(rng);
        bytes.resize(len);
        for (auto& byte : bytes) {
            byte = static_cast<uint8_t>(byte_dist(rng));
        }
        (void)codec.decode(bytes.data(), bytes.size(), 1);
    }

    std::cout << "protocol codec fuzz harness completed " << iterations << " iterations\n";
    return 0;
}
