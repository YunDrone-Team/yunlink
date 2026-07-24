/** @file @brief Deterministic little-endian payload IO used by Core codecs. */

#ifndef YUNLINK_CORE_BINARY_CODEC_IO_HPP
#define YUNLINK_CORE_BINARY_CODEC_IO_HPP

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <utility>

#include "yunlink/core/configuration_codec.hpp"

namespace yunlink {

constexpr size_t kMaxCoreStringBytes = 1024;

struct BufferWriter {
    ByteBuffer data;
    bool valid = true;

    void invalidate() {
        valid = false;
    }

    void write_u8(uint8_t value) {
        data.push_back(value);
    }

    void write_bool(bool value) {
        write_u8(value ? 1U : 0U);
    }

    void write_u16(uint16_t value) {
        data.push_back(static_cast<uint8_t>(value));
        data.push_back(static_cast<uint8_t>(value >> 8U));
    }

    void write_u64(uint64_t value) {
        for (unsigned shift = 0; shift < 64; shift += 8) {
            data.push_back(static_cast<uint8_t>(value >> shift));
        }
    }

    void write_double(double value) {
        uint64_t bits = 0;
        static_assert(sizeof(bits) == sizeof(value), "double must be IEEE-754 binary64");
        std::memcpy(&bits, &value, sizeof(bits));
        write_u64(bits);
    }

    void write_string(const std::string& value) {
        if (value.size() > kMaxCoreStringBytes) {
            invalidate();
            return;
        }
        write_u16(static_cast<uint16_t>(value.size()));
        data.insert(data.end(), value.begin(), value.end());
    }
};

struct BufferReader {
    const ByteBuffer& data;
    size_t cursor = 0;

    bool read_u8(uint8_t* out) {
        if (out == nullptr || cursor + 1 > data.size()) {
            return false;
        }
        *out = data[cursor++];
        return true;
    }

    bool read_bool(bool* out) {
        uint8_t value = 0;
        if (out == nullptr || !read_u8(&value) || value > 1U) {
            return false;
        }
        *out = value != 0;
        return true;
    }

    bool read_u16(uint16_t* out) {
        if (out == nullptr || cursor + 2 > data.size()) {
            return false;
        }
        *out = static_cast<uint16_t>(data[cursor]) | static_cast<uint16_t>(data[cursor + 1] << 8U);
        cursor += 2;
        return true;
    }

    bool read_u64(uint64_t* out) {
        if (out == nullptr || cursor + 8 > data.size()) {
            return false;
        }
        *out = 0;
        for (unsigned shift = 0; shift < 64; shift += 8) {
            *out |= static_cast<uint64_t>(data[cursor++]) << shift;
        }
        return true;
    }

    bool read_double(double* out) {
        uint64_t bits = 0;
        if (out == nullptr || !read_u64(&bits)) {
            return false;
        }
        std::memcpy(out, &bits, sizeof(bits));
        return true;
    }

    bool read_string(std::string* out) {
        uint16_t size = 0;
        if (out == nullptr || !read_u16(&size) || cursor + size > data.size()) {
            return false;
        }
        out->assign(reinterpret_cast<const char*>(data.data() + cursor), size);
        cursor += size;
        return true;
    }

    bool done() const {
        return cursor == data.size();
    }
};

template <typename Fn> ByteBuffer build_payload(Fn&& fn) {
    BufferWriter writer;
    fn(writer);
    return writer.valid ? std::move(writer.data) : ByteBuffer{};
}

template <typename T, typename Fn> bool parse_payload(const ByteBuffer& bytes, T* out, Fn&& fn) {
    if (out == nullptr) {
        return false;
    }
    BufferReader reader{bytes};
    T parsed{};
    if (!fn(reader, &parsed) || !reader.done()) {
        return false;
    }
    *out = std::move(parsed);
    return true;
}

}  // namespace yunlink

#endif  // YUNLINK_CORE_BINARY_CODEC_IO_HPP
