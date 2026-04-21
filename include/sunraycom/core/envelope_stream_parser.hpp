/**
 * @file include/sunraycom/core/envelope_stream_parser.hpp
 * @brief 字节流信封解析器定义。
 */

#ifndef SUNRAYCOM_CORE_ENVELOPE_STREAM_PARSER_HPP
#define SUNRAYCOM_CORE_ENVELOPE_STREAM_PARSER_HPP

#include <cstddef>
#include <cstdint>

#include "sunraycom/core/protocol_codec.hpp"

namespace sunraycom {

class EnvelopeStreamParser {
  public:
    explicit EnvelopeStreamParser(size_t max_buffer_bytes = (1U << 20));

    void feed(const uint8_t* data, size_t len);
    void feed(const ByteBuffer& data);
    bool pop_next(SecureEnvelope* out, DecodeResult* result = nullptr);
    void clear();
    size_t size() const;

  private:
    ByteBuffer buffer_;
    size_t max_buffer_bytes_;
    ProtocolCodec codec_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_ENVELOPE_STREAM_PARSER_HPP
