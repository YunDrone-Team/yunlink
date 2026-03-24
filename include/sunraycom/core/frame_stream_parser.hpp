/**
 * @file include/sunraycom/core/frame_stream_parser.hpp
 * @brief 字节流帧解析器定义。
 */

#ifndef SUNRAYCOM_CORE_FRAME_STREAM_PARSER_HPP
#define SUNRAYCOM_CORE_FRAME_STREAM_PARSER_HPP

#include <cstddef>
#include <cstdint>

#include "sunraycom/core/protocol_codec.hpp"

namespace sunraycom {

/**
 * @brief 增量式协议帧流解析器。
 */
class FrameStreamParser {
  public:
    /**
     * @brief 构造解析器。
     * @param max_buffer_bytes 最大缓存字节数。
     */
    explicit FrameStreamParser(size_t max_buffer_bytes = (1U << 20));

    /**
     * @brief 投喂原始字节数据。
     * @param data 数据指针。
     * @param len 数据长度。
     */
    void feed(const uint8_t* data, size_t len);
    /**
     * @brief 投喂字节缓冲。
     * @param data 字节缓冲。
     */
    void feed(const ByteBuffer& data);

    /**
     * @brief 解析并弹出下一帧。
     * @param out 输出帧。
     * @param result 输出解码结果，可空。
     * @return true 表示成功弹出一帧。
     */
    bool pop_next(Frame* out, DecodeResult* result = nullptr);

    /**
     * @brief 清空内部缓冲。
     */
    void clear();
    /**
     * @brief 返回内部缓冲字节数。
     * @return 当前缓存长度。
     */
    size_t size() const;

  private:
    ByteBuffer buffer_;
    size_t max_buffer_bytes_;
    ProtocolCodec codec_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_FRAME_STREAM_PARSER_HPP
