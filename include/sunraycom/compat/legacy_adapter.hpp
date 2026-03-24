/**
 * @file include/sunraycom/compat/legacy_adapter.hpp
 * @brief legacy 协议适配器定义。
 */

#ifndef SUNRAYCOM_COMPAT_LEGACY_ADAPTER_HPP
#define SUNRAYCOM_COMPAT_LEGACY_ADAPTER_HPP

#include <optional>

#include "sunraycom/core/protocol_codec.hpp"

// Legacy headers are intentionally global to preserve legacy struct names.
#include "LegacyCodec.h"
#include "MSG.h"

namespace sunraycom::compat {

/**
 * @brief legacy DataFrame 与核心 Frame 之间的适配器。
 */
class LegacyAdapter {
  public:
    LegacyAdapter() = default;

    /**
     * @brief 编码 legacy 数据。
     * @param frame legacy 帧。
     * @return 编码后的字节流。
     */
    ByteBuffer encode_legacy(const ::DataFrame& frame) const;
    /**
     * @brief 解码 legacy 数据。
     * @param bytes 输入字节流。
     * @param out 输出 legacy 帧。
     * @return true 表示解码成功。
     */
    bool decode_legacy(const ByteBuffer& bytes, ::DataFrame* out) const;

    /**
     * @brief legacy 帧转核心帧。
     * @param legacy_frame legacy 帧。
     * @return 核心帧；失败时为空。
     */
    std::optional<Frame> to_core_frame(const ::DataFrame& legacy_frame) const;
    /**
     * @brief 核心帧转 legacy 帧。
     * @param frame 核心帧。
     * @param out 输出 legacy 帧。
     * @return true 表示转换成功。
     */
    bool from_core_frame(const Frame& frame, ::DataFrame* out) const;

  private:
    mutable ::LegacyCodec legacy_codec_;
    ProtocolCodec core_codec_;
};

}  // namespace sunraycom::compat

#endif  // SUNRAYCOM_COMPAT_LEGACY_ADAPTER_HPP
