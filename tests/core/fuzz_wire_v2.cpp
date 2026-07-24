#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <vector>

#include "yunlink/core/wire_v2_codec.hpp"

int main(int argc, char** argv) {
    std::vector<uint8_t> bytes;
    if (argc > 1) {
        std::ifstream input(argv[1], std::ios::binary);
        bytes.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
    } else {
        bytes.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }
    yunlink::v2::WireCodec codec;
    (void)codec.decode(bytes.data(), bytes.size(), 1);
    return 0;
}
