/**
 * @file src/core/frame_stream_parser.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/core/frame_stream_parser.hpp"

#include <algorithm>
#include <string>

namespace sunraycom {

FrameStreamParser::FrameStreamParser(size_t max_buffer_bytes)
    : max_buffer_bytes_(max_buffer_bytes) {}

void FrameStreamParser::feed(const uint8_t* data, size_t len) {
    if (data == nullptr || len == 0)
        return;
    buffer_.insert(buffer_.end(), data, data + len);
    if (buffer_.size() > max_buffer_bytes_) {
        // Keep the tail to avoid unbounded memory usage.
        buffer_.erase(buffer_.begin(),
                      buffer_.begin() + static_cast<long>(buffer_.size() - max_buffer_bytes_));
    }
}

void FrameStreamParser::feed(const ByteBuffer& data) {
    feed(data.data(), data.size());
}

bool FrameStreamParser::pop_next(Frame* out, DecodeResult* result) {
    while (buffer_.size() >= 2) {
        size_t head_pos = std::string::npos;
        for (size_t i = 0; i + 1 < buffer_.size(); ++i) {
            const uint16_t head =
                static_cast<uint16_t>(buffer_[i + 1]) | (static_cast<uint16_t>(buffer_[i]) << 8);
            if (ProtocolCodec::is_valid_head(head)) {
                head_pos = i;
                break;
            }
        }

        if (head_pos == std::string::npos) {
            if (buffer_.size() > 1) {
                buffer_.erase(buffer_.begin(), buffer_.end() - 1);
            }
            return false;
        }

        if (head_pos > 0) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<long>(head_pos));
        }

        if (buffer_.size() < 6) {
            return false;
        }

        uint32_t frame_len = 0;
        for (int i = 0; i < 4; ++i) {
            frame_len |= static_cast<uint32_t>(buffer_[2 + i]) << (i * 8);
        }

        if (frame_len < ProtocolCodec::kMinFrameSize) {
            buffer_.erase(buffer_.begin(), buffer_.begin() + 2);
            continue;
        }

        if (buffer_.size() < frame_len) {
            return false;
        }

        const DecodeResult dr = codec_.decode(buffer_.data(), frame_len);
        if (!dr.ok()) {
            if (result)
                *result = dr;
            // Skip current head and search next one.
            buffer_.erase(buffer_.begin(), buffer_.begin() + 2);
            continue;
        }

        if (out)
            *out = dr.frame;
        if (result)
            *result = dr;
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<long>(dr.consumed));
        return true;
    }
    return false;
}

void FrameStreamParser::clear() {
    buffer_.clear();
}

size_t FrameStreamParser::size() const {
    return buffer_.size();
}

}  // namespace sunraycom
