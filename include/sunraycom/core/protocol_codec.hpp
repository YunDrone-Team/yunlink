/**
 * @file include/sunraycom/core/protocol_codec.hpp
 * @brief 协议编解码接口定义。
 */

#ifndef SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP
#define SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP

#include <cstddef>
#include <cstdint>

#include "sunraycom/core/types.hpp"

namespace sunraycom {

class ProtocolCodec {
  public:
    static constexpr uint8_t kMagic0 = 'S';
    static constexpr uint8_t kMagic1 = 'R';
    static constexpr uint8_t kMagic2 = 'C';
    static constexpr uint8_t kMagic3 = 'M';

    static constexpr uint16_t kFixedHeaderSize = 76;
    static constexpr uint32_t kTrailerSize = 4;
    static constexpr uint32_t kMinEnvelopeSize = kFixedHeaderSize + kTrailerSize;

    static bool has_magic(const uint8_t* data, size_t len);
    static uint32_t checksum(const uint8_t* data, size_t len);

    ByteBuffer encode(const SecureEnvelope& envelope, bool auto_fill_header = true) const;
    DecodeResult decode(const uint8_t* data, size_t len, uint64_t now_ms = 0) const;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP
