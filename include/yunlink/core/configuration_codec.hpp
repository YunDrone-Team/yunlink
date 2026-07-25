#ifndef YUNLINK_CORE_CONFIGURATION_CODEC_HPP
#define YUNLINK_CORE_CONFIGURATION_CODEC_HPP

#include <cstdint>
#include <vector>

#include "yunlink/core/semantic/configuration/service_types.hpp"

namespace yunlink {

using ByteBuffer = std::vector<uint8_t>;

ByteBuffer encode_payload(const ConfigResourceListRequest& value);
ByteBuffer encode_payload(const ConfigResourceListResponse& value);
ByteBuffer encode_payload(const ConfigResourceDescribeRequest& value);
ByteBuffer encode_payload(const ConfigResourceDescribeResponse& value);
ByteBuffer encode_payload(const ConfigResourceGetRequest& value);
ByteBuffer encode_payload(const ConfigResourceGetResponse& value);
ByteBuffer encode_payload(const ConfigResourcePatchRequest& value);
ByteBuffer encode_payload(const ConfigResourcePatchResponse& value);
ByteBuffer encode_payload(const ConfigResourceApplyRequest& value);
ByteBuffer encode_payload(const ConfigResourceApplyResponse& value);

bool decode_payload(const ByteBuffer& bytes, ConfigResourceListRequest* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceListResponse* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceDescribeRequest* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceDescribeResponse* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceGetRequest* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceGetResponse* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourcePatchRequest* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourcePatchResponse* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceApplyRequest* value);
bool decode_payload(const ByteBuffer& bytes, ConfigResourceApplyResponse* value);

}  // namespace yunlink

#endif  // YUNLINK_CORE_CONFIGURATION_CODEC_HPP
