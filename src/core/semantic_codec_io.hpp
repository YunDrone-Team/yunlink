/**
 * @file src/core/semantic_codec_io.hpp
 * @brief 语义消息编解码内部 IO helper。
 */

#ifndef SUNRAYCOM_CORE_SEMANTIC_CODEC_IO_HPP
#define SUNRAYCOM_CORE_SEMANTIC_CODEC_IO_HPP

#include "sunraycom/core/semantic_messages.hpp"

namespace sunraycom {

struct BufferWriter {
    ByteBuffer data;

    void write_u8(uint8_t value) {
        data.push_back(value);
    }

    void write_i8(int8_t value) {
        write_u8(static_cast<uint8_t>(value));
    }

    void write_bool(bool value) {
        write_u8(value ? 1U : 0U);
    }

    void write_u16(uint16_t value) {
        data.push_back(static_cast<uint8_t>(value & 0xFFU));
        data.push_back(static_cast<uint8_t>((value >> 8) & 0xFFU));
    }

    void write_u32(uint32_t value) {
        for (int i = 0; i < 4; ++i) {
            data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
        }
    }

    void write_u64(uint64_t value) {
        for (int i = 0; i < 8; ++i) {
            data.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFFU));
        }
    }

    void write_float(float value) {
        const auto raw = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), raw, raw + sizeof(float));
    }

    void write_double(double value) {
        const auto raw = reinterpret_cast<const uint8_t*>(&value);
        data.insert(data.end(), raw, raw + sizeof(double));
    }

    void write_string(const std::string& value) {
        write_u16(static_cast<uint16_t>(value.size()));
        data.insert(data.end(), value.begin(), value.end());
    }
};

struct BufferReader {
    const ByteBuffer& data;
    size_t cursor = 0;

    bool read_u8(uint8_t* out) {
        if (cursor + 1 > data.size()) {
            return false;
        }
        *out = data[cursor++];
        return true;
    }

    bool read_i8(int8_t* out) {
        uint8_t value = 0;
        return read_u8(&value) ? (*out = static_cast<int8_t>(value), true) : false;
    }

    bool read_bool(bool* out) {
        uint8_t value = 0;
        return read_u8(&value) ? (*out = value != 0, true) : false;
    }

    bool read_u16(uint16_t* out) {
        if (cursor + 2 > data.size()) {
            return false;
        }
        *out = static_cast<uint16_t>(data[cursor]) | (static_cast<uint16_t>(data[cursor + 1]) << 8);
        cursor += 2;
        return true;
    }

    bool read_u32(uint32_t* out) {
        if (cursor + 4 > data.size()) {
            return false;
        }
        *out = static_cast<uint32_t>(data[cursor]) |
               (static_cast<uint32_t>(data[cursor + 1]) << 8) |
               (static_cast<uint32_t>(data[cursor + 2]) << 16) |
               (static_cast<uint32_t>(data[cursor + 3]) << 24);
        cursor += 4;
        return true;
    }

    bool read_u64(uint64_t* out) {
        if (cursor + 8 > data.size()) {
            return false;
        }
        *out = 0;
        for (int i = 0; i < 8; ++i) {
            *out |= static_cast<uint64_t>(data[cursor + i]) << (i * 8);
        }
        cursor += 8;
        return true;
    }

    bool read_float(float* out) {
        if (cursor + sizeof(float) > data.size()) {
            return false;
        }
        *out = *reinterpret_cast<const float*>(data.data() + cursor);
        cursor += sizeof(float);
        return true;
    }

    bool read_double(double* out) {
        if (cursor + sizeof(double) > data.size()) {
            return false;
        }
        *out = *reinterpret_cast<const double*>(data.data() + cursor);
        cursor += sizeof(double);
        return true;
    }

    bool read_string(std::string* out) {
        uint16_t size = 0;
        if (!read_u16(&size) || cursor + size > data.size()) {
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
    return writer.data;
}

template <typename T, typename Fn> bool parse_payload(const ByteBuffer& bytes, T* out, Fn&& fn) {
    if (out == nullptr) {
        return false;
    }
    BufferReader reader{bytes};
    return fn(reader, out) && reader.done();
}

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_SEMANTIC_CODEC_IO_HPP
