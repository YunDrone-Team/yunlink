#ifndef YUNLINK_CORE_CONFIGURATION_CODEC_HPP
#define YUNLINK_CORE_CONFIGURATION_CODEC_HPP

#include <cstdint>
#include <vector>

#include "yunlink/core/semantic/configuration/service_types.hpp"

namespace yunlink {

using ByteBuffer = std::vector<uint8_t>;

#define YUNLINK_CONFIG_CODEC(Type)                  \
    ByteBuffer encode_payload(const Type& value);   \
    bool decode_payload(const ByteBuffer& bytes, Type* value)

YUNLINK_CONFIG_CODEC(ConfigResourceListRequest);
YUNLINK_CONFIG_CODEC(ConfigResourceListResponse);
YUNLINK_CONFIG_CODEC(ConfigResourceDescribeRequest);
YUNLINK_CONFIG_CODEC(ConfigResourceDescribeResponse);
YUNLINK_CONFIG_CODEC(ConfigResourceGetRequest);
YUNLINK_CONFIG_CODEC(ConfigResourceGetResponse);
YUNLINK_CONFIG_CODEC(ConfigResourcePatchRequest);
YUNLINK_CONFIG_CODEC(ConfigResourcePatchResponse);
YUNLINK_CONFIG_CODEC(ConfigResourceApplyRequest);
YUNLINK_CONFIG_CODEC(ConfigResourceApplyResponse);

#undef YUNLINK_CONFIG_CODEC

}  // namespace yunlink

#endif  // YUNLINK_CORE_CONFIGURATION_CODEC_HPP
