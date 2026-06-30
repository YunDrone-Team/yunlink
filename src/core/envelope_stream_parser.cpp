/**
 * @file src/core/envelope_stream_parser.cpp
 * @brief yunlink source file.
 */

#include "yunlink/core/envelope_stream_parser.hpp"

#include <algorithm>

namespace yunlink {

namespace {

void assign_raw_preview(EnvelopeStreamParseEvent* event,
                        const ByteBuffer& buffer,
                        size_t raw_len,
                        size_t limit) {
    if (event == nullptr) {
        return;
    }
    event->raw_len = raw_len;
    event->raw_truncated = raw_len > limit;
    const size_t keep = std::min(raw_len, limit);
    event->raw.assign(buffer.begin(), buffer.begin() + static_cast<long>(keep));
}

std::string decode_error_label(ErrorCode code) {
    switch (code) {
    case ErrorCode::kChecksumMismatch:
        return "checksum-mismatch";
    case ErrorCode::kInvalidHeader:
        return "invalid-header";
    case ErrorCode::kTimeout:
        return "ttl-expired";
    case ErrorCode::kDecodeError:
        return "decode-error";
    default:
        break;
    }
    return "decode-failed";
}

}  // namespace

EnvelopeStreamParser::EnvelopeStreamParser(size_t max_buffer_bytes, size_t max_frame_bytes)
    : max_buffer_bytes_(max_buffer_bytes), max_frame_bytes_(max_frame_bytes) {}

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

bool EnvelopeStreamParser::pop_next_event(EnvelopeStreamParseEvent* event,
                                          size_t raw_preview_limit) {
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
        if (header_len < ProtocolCodec::kFixedHeaderSize) {
            if (event != nullptr) {
                event->has_envelope = false;
                event->result.code = ErrorCode::kDecodeError;
                event->result.consumed = 1;
                event->error_message = "invalid-header-len";
                assign_raw_preview(
                    event, buffer_, std::min<size_t>(buffer_.size(), 14), raw_preview_limit);
            }
            buffer_.erase(buffer_.begin());
            return event != nullptr;
        }
        const size_t frame_prefix_len =
            static_cast<size_t>(header_len) + ProtocolCodec::kTrailerSize;
        const bool oversized =
            static_cast<size_t>(header_len) > max_frame_bytes_ ||
            static_cast<size_t>(payload_len) > max_frame_bytes_ ||
            frame_prefix_len > max_frame_bytes_ ||
            static_cast<size_t>(payload_len) > max_frame_bytes_ - frame_prefix_len;
        if (oversized) {
            if (event != nullptr) {
                event->has_envelope = false;
                event->result.code = ErrorCode::kDecodeError;
                event->result.consumed = 1;
                event->error_message = "oversized-frame";
                assign_raw_preview(
                    event, buffer_, std::min<size_t>(buffer_.size(), 14), raw_preview_limit);
            }
            buffer_.erase(buffer_.begin());
            return event != nullptr;
        }

        const size_t total_len = frame_prefix_len + static_cast<size_t>(payload_len);
        if (buffer_.size() < total_len) {
            return false;
        }

        const DecodeResult dr = codec_.decode(buffer_.data(), total_len);
        if (!dr.ok()) {
            if (event != nullptr) {
                event->has_envelope = false;
                event->result = dr;
                event->error_message = decode_error_label(dr.code);
                assign_raw_preview(event, buffer_, total_len, raw_preview_limit);
            }
            buffer_.erase(buffer_.begin());
            return event != nullptr;
        }

        if (event != nullptr) {
            event->has_envelope = true;
            event->result = dr;
            assign_raw_preview(event, buffer_, dr.consumed, raw_preview_limit);
        }
        buffer_.erase(buffer_.begin(), buffer_.begin() + static_cast<long>(dr.consumed));
        return true;
    }
    return false;
}

bool EnvelopeStreamParser::pop_next(SecureEnvelope* out, DecodeResult* result) {
    EnvelopeStreamParseEvent event;
    while (pop_next_event(&event, 0)) {
        if (result != nullptr) {
            *result = event.result;
        }
        if (!event.has_envelope || !event.result.ok()) {
            continue;
        }
        if (out != nullptr) {
            *out = event.result.envelope;
        }
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

}  // namespace yunlink
