/**
 * @file include/sunraycom/core/protocol_codec.hpp
 * @brief 协议编解码接口定义。
 */

#ifndef SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP
#define SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP

#include <cstddef>
#include <cstdint>

#include "sunraycom/core/message_ids.hpp"
#include "sunraycom/core/types.hpp"

namespace sunraycom {

/**
 * @brief Sunray 协议编解码器。
 */
class ProtocolCodec {
  public:
    static constexpr uint16_t kHeadTcp = 0xac43;
    static constexpr uint16_t kHeadUdpState = 0xab65;
    static constexpr uint16_t kHeadUdpSearch = 0xad21;
    static constexpr uint16_t kHeadUdpAck = 0xfd32;
    static constexpr uint16_t kHeadShared = 0xcc90;

    static constexpr uint32_t kMinFrameSize = 18;

    /**
     * @brief 检查帧头是否合法。
     * @param head 帧头。
     * @return true 表示合法。
     */
    static bool is_valid_head(uint16_t head);
    /**
     * @brief 根据消息序号推断帧头。
     * @param seq 消息序号。
     * @return 推断得到的帧头。
     */
    static uint16_t infer_head(uint8_t seq);
    /**
     * @brief 计算校验和。
     * @param data 输入数据。
     * @param len 数据长度。
     * @return 16 位校验和。
     */
    static uint16_t checksum(const uint8_t* data, size_t len);

    /**
     * @brief 编码帧为字节流。
     * @param frame 输入帧。
     * @param auto_fill_header 是否自动填充帧头字段。
     * @return 编码后的字节流。
     */
    ByteBuffer encode(const Frame& frame, bool auto_fill_header = true) const;
    /**
     * @brief 从字节流解码帧。
     * @param data 输入字节数据。
     * @param len 数据长度。
     * @return 解码结果。
     */
    DecodeResult decode(const uint8_t* data, size_t len) const;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_PROTOCOL_CODEC_HPP
