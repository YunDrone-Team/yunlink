/**
 * @file include/yunlink/core/core_messages_v2.hpp
 * @brief Deterministic payloads owned by YunLink Core.
 */

#ifndef YUNLINK_CORE_CORE_MESSAGES_V2_HPP
#define YUNLINK_CORE_CORE_MESSAGES_V2_HPP

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "yunlink/core/wire_v2.hpp"
#include "yunlink/core/semantic/configuration/service_types.hpp"

namespace yunlink::v2 {

struct EntityDirectory {
    std::string endpoint_uid;
    std::string revision;
    std::vector<EntityDescriptor> entities;
};

struct AttachmentRequest {
    std::string expected_revision;
    std::vector<std::string> entity_uids;
};

struct AttachmentResponse {
    bool success = false;
    std::string revision;
    std::vector<std::string> attached_entity_uids;
    std::string message;
};

struct AuthorityRequest {
    std::string authority_scope;
    uint32_t lease_ttl_ms = 0;
    bool allow_preempt = false;
};

struct AuthorityStatus {
    std::string authority_scope;
    std::string state;
    uint32_t lease_ttl_ms = 0;
    uint16_t reason_code = 0;
    std::string detail;
};

struct StreamDescriptor {
    std::string stream_uid;
    TypeRef type;
    std::string encoding;
    std::map<std::string, std::string> metadata;
};

struct StreamCatalog {
    std::string revision;
    std::vector<StreamDescriptor> streams;
};

struct StreamSubscription {
    std::string stream_uid;
    float max_rate_hz = 0.0F;
    uint32_t max_payload_bytes = 0;
};

struct StreamSample {
    std::string stream_uid;
    std::string encoding;
    std::map<std::string, std::string> metadata;
    uint64_t source_timestamp_ns = 0;
    uint64_t sequence = 0;
    Bytes data;
};

struct ActionUpdate {
    ActionPhase phase = ActionPhase::kReceived;
    uint16_t result_code = 0;
    uint8_t progress_percent = 0;
    std::string detail;
};

struct BulkDescriptor {
    std::string media_type;
    std::string encoding;
    std::map<std::string, std::string> metadata;
    uint64_t total_bytes = 0;
};

struct LogSummary {
    std::string log_uid;
    std::string owner_uid;
    std::string title;
    std::string state;
    uint64_t started_at_ns = 0;
    uint64_t finished_at_ns = 0;
    bool has_exit_code = false;
    int32_t exit_code = 0;
    std::map<std::string, std::string> labels;
    std::string message;
};

struct LogListResponse {
    bool success = false;
    std::string message;
    std::vector<LogSummary> logs;
};

struct LogReadRequest {
    std::string log_uid;
    uint64_t cursor = 0;
    uint32_t max_bytes = 0;
};

struct LogReadResponse {
    bool success = false;
    std::string message;
    std::string log_uid;
    std::string chunk;
    uint64_t next_cursor = 0;
    bool truncated = false;
    bool eof = false;
};

Bytes encode(const EntityDirectory& value);
Bytes encode(const AttachmentRequest& value);
Bytes encode(const AttachmentResponse& value);
Bytes encode(const AuthorityRequest& value);
Bytes encode(const AuthorityStatus& value);
Bytes encode(const StreamCatalog& value);
Bytes encode(const StreamSubscription& value);
Bytes encode(const StreamSample& value);
Bytes encode(const ActionUpdate& value);
Bytes encode(const BulkDescriptor& value);
Bytes encode(const ConfigResourceListRequest& value);
Bytes encode(const ConfigResourceListResponse& value);
Bytes encode(const ConfigResourceDescribeRequest& value);
Bytes encode(const ConfigResourceDescribeResponse& value);
Bytes encode(const ConfigResourceGetRequest& value);
Bytes encode(const ConfigResourceGetResponse& value);
Bytes encode(const ConfigResourcePatchRequest& value);
Bytes encode(const ConfigResourcePatchResponse& value);
Bytes encode(const ConfigResourceApplyRequest& value);
Bytes encode(const ConfigResourceApplyResponse& value);
Bytes encode(const ConfigResourceVariantListRequest& value);
Bytes encode(const ConfigResourceVariantListResponse& value);
Bytes encode(const ConfigResourceVariantCreateRequest& value);
Bytes encode(const ConfigResourceVariantCreateResponse& value);
Bytes encode(const ConfigResourceVariantSaveCurrentRequest& value);
Bytes encode(const ConfigResourceVariantSaveCurrentResponse& value);
Bytes encode(const ConfigResourceVariantActivateRequest& value);
Bytes encode(const ConfigResourceVariantActivateResponse& value);
Bytes encode(const ConfigResourceVariantDeleteRequest& value);
Bytes encode(const ConfigResourceVariantDeleteResponse& value);
Bytes encode(const LogListResponse& value);
Bytes encode(const LogReadRequest& value);
Bytes encode(const LogReadResponse& value);

bool decode(const Bytes& bytes, EntityDirectory* value);
bool decode(const Bytes& bytes, AttachmentRequest* value);
bool decode(const Bytes& bytes, AttachmentResponse* value);
bool decode(const Bytes& bytes, AuthorityRequest* value);
bool decode(const Bytes& bytes, AuthorityStatus* value);
bool decode(const Bytes& bytes, StreamCatalog* value);
bool decode(const Bytes& bytes, StreamSubscription* value);
bool decode(const Bytes& bytes, StreamSample* value);
bool decode(const Bytes& bytes, ActionUpdate* value);
bool decode(const Bytes& bytes, BulkDescriptor* value);
bool decode(const Bytes& bytes, ConfigResourceListRequest* value);
bool decode(const Bytes& bytes, ConfigResourceListResponse* value);
bool decode(const Bytes& bytes, ConfigResourceDescribeRequest* value);
bool decode(const Bytes& bytes, ConfigResourceDescribeResponse* value);
bool decode(const Bytes& bytes, ConfigResourceGetRequest* value);
bool decode(const Bytes& bytes, ConfigResourceGetResponse* value);
bool decode(const Bytes& bytes, ConfigResourcePatchRequest* value);
bool decode(const Bytes& bytes, ConfigResourcePatchResponse* value);
bool decode(const Bytes& bytes, ConfigResourceApplyRequest* value);
bool decode(const Bytes& bytes, ConfigResourceApplyResponse* value);
bool decode(const Bytes& bytes, ConfigResourceVariantListRequest* value);
bool decode(const Bytes& bytes, ConfigResourceVariantListResponse* value);
bool decode(const Bytes& bytes, ConfigResourceVariantCreateRequest* value);
bool decode(const Bytes& bytes, ConfigResourceVariantCreateResponse* value);
bool decode(const Bytes& bytes, ConfigResourceVariantSaveCurrentRequest* value);
bool decode(const Bytes& bytes, ConfigResourceVariantSaveCurrentResponse* value);
bool decode(const Bytes& bytes, ConfigResourceVariantActivateRequest* value);
bool decode(const Bytes& bytes, ConfigResourceVariantActivateResponse* value);
bool decode(const Bytes& bytes, ConfigResourceVariantDeleteRequest* value);
bool decode(const Bytes& bytes, ConfigResourceVariantDeleteResponse* value);
bool decode(const Bytes& bytes, LogListResponse* value);
bool decode(const Bytes& bytes, LogReadRequest* value);
bool decode(const Bytes& bytes, LogReadResponse* value);

}  // namespace yunlink::v2

#endif  // YUNLINK_CORE_CORE_MESSAGES_V2_HPP
