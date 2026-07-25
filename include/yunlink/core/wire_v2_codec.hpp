/**
 * @file include/yunlink/core/wire_v2_codec.hpp
 * @brief Deterministic YunLink Wire v2 envelope codec.
 */

#ifndef YUNLINK_CORE_WIRE_V2_CODEC_HPP
#define YUNLINK_CORE_WIRE_V2_CODEC_HPP

#include "yunlink/core/wire_v2.hpp"

namespace yunlink::v2 {

class WireCodec {
  public:
    static constexpr uint8_t kMagic0 = 'Y';
    static constexpr uint8_t kMagic1 = 'L';
    static constexpr uint8_t kMagic2 = 'N';
    static constexpr uint8_t kMagic3 = 'K';
    static constexpr uint16_t kFixedHeaderSize = 80;
    static constexpr uint32_t kTrailerSize = 4;
    static constexpr uint32_t kMinEnvelopeSize = kFixedHeaderSize + kTrailerSize;

    static bool has_magic(const uint8_t* data, size_t len);
    static uint32_t checksum(const uint8_t* data, size_t len);

    Bytes encode(const Envelope& envelope, bool auto_fill_header = true) const;
    DecodeResult decode(const uint8_t* data, size_t len, uint64_t now_ms = 0) const;
};

}  // namespace yunlink::v2

#endif  // YUNLINK_CORE_WIRE_V2_CODEC_HPP
