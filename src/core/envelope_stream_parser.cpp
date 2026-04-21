/**
 * @file src/core/envelope_stream_parser.cpp
 * @brief sunray_communication_lib source file.
 */

#include "sunraycom/core/envelope_stream_parser.hpp"

namespace sunraycom {

EnvelopeStreamParser::EnvelopeStreamParser(size_t max_buffer_bytes)
    : max_buffer_bytes_(max_buffer_bytes) {}

void EnvelopeStreamParser::feed(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0) {
        return;
    }
    buffer_.insert(buffer_.end(), data, data + len);
    if (buffer_.size() > max_buffer_bytes_) {
        buffer_.erase(buffer_.begin(),
                      buffer_.begin() + static_cast<long>(buffer_.size() - max_buffer_bytes_));
    }
}

void EnvelopeStreamParser::feed(const ByteBuffer& data) {
    feed(data.data(), data.size());
}

bool EnvelopeStreamParser::pop_next(SecureEnvelope* out, DecodeResult* result) {
    while (buffer_.size() >= 4) {
        size_t magic_pos = buffer_.size();
        for (size_t i = 0; i + 3 < buffer_.size(); ++i) {
            if (ProtocolCodec::has_magic(buffer_.data() + i, buffer_.size() - i)) {
                magic_pos = i;
                break;
            }
        }

        if (magic_pos == buffer_.size()) {
            buffer_.clear();
            return false;
        }
        if (magic_pos > 0) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<long>(magic_pos));
        }
        if (buffer_.size() < 14) {
            return false;
        }

        const uint16_t header_len =
            static_cast<uint16_t>(buffer_[8]) | (static_cast<uint16_t>(buffer_[9]) << 8);
        const uint32_t payload_len =
            static_cast<uint32_t>(buffer_[10]) | (static_cast<uint32_t>(buffer_[11]) << 8) |
            (static_cast<uint32_t>(buffer_[12]) << 16) | (static_cast<uint32_t>(buffer_[13]) << 24);
        const size_t total_len =
            static_cast<size_t>(header_len) + payload_len + ProtocolCodec::kTrailerSize;
        if (header_len < ProtocolCodec::kFixedHeaderSize) {
            buffer_.erase(buffer_.begin());
            continue;
        }
        if (buffer_.size() < total_len) {
            return false;
        }

        const DecodeResult dr = codec_.decode(buffer_.data(), total_len);
        if (!dr.ok()) {
            if (result) {
                *result = dr;
            }
            buffer_.erase(buffer_.begin());
            continue;
        }

        if (out) {
            *out = dr.envelope;
        }
        if (result) {
            *result = dr;
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<long>(dr.consumed));
        return true;
    }
    return false;
}

void EnvelopeStreamParser::clear() {
    buffer_.clear();
}

size_t EnvelopeStreamParser::size() const {
    return buffer_.size();
}

}  // namespace sunraycom
