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

Bytes encode(const EntityDirectory& value);
Bytes encode(const AttachmentRequest& value);
Bytes encode(const AttachmentResponse& value);
Bytes encode(const AuthorityRequest& value);
Bytes encode(const AuthorityStatus& value);
Bytes encode(const StreamCatalog& value);
Bytes encode(const StreamSubscription& value);
Bytes encode(const ActionUpdate& value);
Bytes encode(const BulkDescriptor& value);

bool decode(const Bytes& bytes, EntityDirectory* value);
bool decode(const Bytes& bytes, AttachmentRequest* value);
bool decode(const Bytes& bytes, AttachmentResponse* value);
bool decode(const Bytes& bytes, AuthorityRequest* value);
bool decode(const Bytes& bytes, AuthorityStatus* value);
bool decode(const Bytes& bytes, StreamCatalog* value);
bool decode(const Bytes& bytes, StreamSubscription* value);
bool decode(const Bytes& bytes, ActionUpdate* value);
bool decode(const Bytes& bytes, BulkDescriptor* value);

}  // namespace yunlink::v2

#endif  // YUNLINK_CORE_CORE_MESSAGES_V2_HPP
