#include "yunlink/core/core_messages_v2.hpp"
#include "yunlink/core/configuration_codec.hpp"

namespace yunlink::v2 {
Bytes encode(const ConfigResourceListRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceListResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceDescribeRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceDescribeResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceGetRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceGetResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourcePatchRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourcePatchResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceApplyRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceApplyResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantListRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantListResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantCreateRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantCreateResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantSaveCurrentRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantSaveCurrentResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantActivateRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantActivateResponse& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantDeleteRequest& value) {
    return ::yunlink::encode_payload(value);
}
Bytes encode(const ConfigResourceVariantDeleteResponse& value) {
    return ::yunlink::encode_payload(value);
}
bool decode(const Bytes& bytes, ConfigResourceListRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceListResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceDescribeRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceDescribeResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceGetRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceGetResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourcePatchRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourcePatchResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceApplyRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceApplyResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantListRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantListResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantCreateRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantCreateResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantSaveCurrentRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantSaveCurrentResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantActivateRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantActivateResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantDeleteRequest* value) {
    return ::yunlink::decode_payload(bytes, value);
}
bool decode(const Bytes& bytes, ConfigResourceVariantDeleteResponse* value) {
    return ::yunlink::decode_payload(bytes, value);
}
}  // namespace yunlink::v2
