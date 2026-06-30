/**
 * @file include/yunlink/core/envelope_stream_parser.hpp
 * @brief 字节流信封解析器定义。
 */

#ifndef YUNLINK_CORE_ENVELOPE_STREAM_PARSER_HPP
#define YUNLINK_CORE_ENVELOPE_STREAM_PARSER_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>

#include "yunlink/core/protocol_codec.hpp"

namespace yunlink {

struct EnvelopeStreamParseEvent {
    bool has_envelope = false;
    DecodeResult result;
    ByteBuffer raw;
    size_t raw_len = 0;
    bool raw_truncated = false;
    std::string error_message;
};

class EnvelopeStreamParser {
  public:
    explicit EnvelopeStreamParser(size_t max_buffer_bytes = (1U << 20),
                                  size_t max_frame_bytes = (1U << 20));

    void feed(const uint8_t* data, size_t len);
    void feed(const ByteBuffer& data);
    bool pop_next_event(EnvelopeStreamParseEvent* event,
                        size_t raw_preview_limit = std::numeric_limits<size_t>::max());
    bool pop_next(SecureEnvelope* out, DecodeResult* result = nullptr);
    void clear();
    size_t size() const;

  private:
    ByteBuffer buffer_;
    size_t max_buffer_bytes_;
    size_t max_frame_bytes_;
    ProtocolCodec codec_;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_ENVELOPE_STREAM_PARSER_HPP
