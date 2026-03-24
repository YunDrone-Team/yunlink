/**
 * @file include/sunraycom/compat/legacy_types.hpp
 * @brief legacy 兼容数据结构定义。
 */

#ifndef SUNRAYCOM_COMPAT_LEGACY_TYPES_HPP
#define SUNRAYCOM_COMPAT_LEGACY_TYPES_HPP

#include <string>

#include "DecoderInterfaceBase.h"

namespace sunraycom::compat {

/**
 * @brief legacy 连接参数结构。
 * @note 字段命名保持 legacy 兼容，不做 snake_case 重命名。
 */
struct ConnectStr {
    unsigned short ListeningPort = 0;
    std::string TargetIP;
    unsigned short TargetPort = 0;
    bool State = false;
    DecoderInterfaceBase* decoderInterfacePtr = nullptr;
};

/**
 * @brief legacy TCP 客户端错误结构。
 * @note 字段命名保持 legacy 兼容，不做 snake_case 重命名。
 */
struct TCPClientErrorStr {
    int error = 0;
    std::string targetIP;
    unsigned short targetPort = 0;
};

}  // namespace sunraycom::compat

#endif  // SUNRAYCOM_COMPAT_LEGACY_TYPES_HPP
