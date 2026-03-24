/**
 * @file include/sunraycom/core/payload_mapper.hpp
 * @brief 负载与 legacy 数据模型映射器。
 */

#ifndef SUNRAYCOM_CORE_PAYLOAD_MAPPER_HPP
#define SUNRAYCOM_CORE_PAYLOAD_MAPPER_HPP

#include <optional>

#include "sunraycom/compat/legacy_adapter.hpp"

namespace sunraycom {

// v1 payload mapper keeps protocol compatibility by delegating to legacy DataFrame mapping.
/**
 * @brief legacy 数据模型与核心帧之间的映射工具。
 */
class PayloadMapper {
  public:
    /**
     * @brief 将 legacy 数据映射为核心帧。
     * @param legacy legacy 数据结构。
     * @return 映射后的核心帧；失败时为空。
     */
    std::optional<Frame> map_legacy_to_frame(const ::DataFrame& legacy) const {
        return adapter_.to_core_frame(legacy);
    }

    /**
     * @brief 将核心帧映射为 legacy 数据。
     * @param frame 核心帧。
     * @param out 输出 legacy 数据。
     * @return true 表示映射成功。
     */
    bool map_frame_to_legacy(const Frame& frame, ::DataFrame* out) const {
        return adapter_.from_core_frame(frame, out);
    }

  private:
    compat::LegacyAdapter adapter_;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_PAYLOAD_MAPPER_HPP
