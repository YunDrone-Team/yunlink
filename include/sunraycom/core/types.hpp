/**
 * @file include/sunraycom/core/types.hpp
 * @brief SunrayComLib 核心类型定义。
 */

#ifndef SUNRAYCOM_CORE_TYPES_HPP
#define SUNRAYCOM_CORE_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace sunraycom {

/**
 * @brief 二进制字节缓冲区类型。
 */
using ByteBuffer = std::vector<uint8_t>;

/**
 * @brief 传输通道类型。
 */
enum class TransportType : uint8_t {
    kTcpServer = 1,
    kTcpClient = 2,
    kUdpUnicast = 3,
    kUdpBroadcast = 4,
};

/**
 * @brief 通用错误码定义。
 */
enum class ErrorCode : uint16_t {
    kOk = 0,
    kInvalidArgument,
    kSocketError,
    kBindError,
    kListenError,
    kConnectError,
    kTimeout,
    kEncodeError,
    kDecodeError,
    kChecksumMismatch,
    kInvalidHeader,
    kRuntimeStopped,
    kInternal,
};

/**
 * @brief 对端标识信息。
 */
struct PeerInfo {
    std::string id;
    std::string ip;
    uint16_t port = 0;
};

/**
 * @brief 协议帧头。
 */
struct FrameHeader {
    uint16_t head = 0;
    uint32_t length = 0;
    uint8_t seq = 0;
    uint8_t robot_id = 0;
    uint64_t timestamp = 0;
    uint16_t checksum = 0;
};

/**
 * @brief 协议帧。
 */
struct Frame {
    FrameHeader header;
    ByteBuffer payload;
};

/**
 * @brief 解码结果。
 */
struct DecodeResult {
    ErrorCode code = ErrorCode::kOk;
    size_t consumed = 0;
    Frame frame;

    /**
     * @brief 判断解码是否成功。
     * @return true 表示成功。
     */
    bool ok() const {
        return code == ErrorCode::kOk;
    }
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_TYPES_HPP
