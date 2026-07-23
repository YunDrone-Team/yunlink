/**
 * @file include/yunlink/core/semantic/system_service_types.hpp
 * @brief Semantic system service payload models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "yunlink/core/types.hpp"

namespace yunlink {

struct FeatureListRequest {
    uint8_t reserved = 0;
};

/** Complete, display-ready description of one managed Sunray feature. */
struct FeatureDescriptor {
    std::string name;
    std::string display_name;
    std::string group_name;
    std::string group_display_name;
    std::string description;
    bool core_feature = false;
    bool example_feature = false;
    bool basic_feature = false;
    bool auto_start = false;
    bool check_feature_state = false;
    uint8_t runtime_state = 0;
    std::string runtime_error;
    std::vector<std::string> depends_on;
    std::vector<std::string> start_preview_units;
    std::vector<std::string> start_preview_commands;
};

struct FeatureListResponse {
    bool success = false;
    std::string message;
    /** Retained for lightweight non-GCS consumers. `features` is authoritative. */
    std::vector<std::string> feature_names;
    std::vector<FeatureDescriptor> features;
};

struct FeatureGetRequest {
    std::string feature_name;
};

struct FeatureGetResponse {
    bool success = false;
    std::string message;
    std::string name;
    std::string title;
    std::string group;
    bool running = false;
    std::string description;
    bool auto_start = false;
    std::vector<std::string> depends_on;
    std::vector<std::string> start_preview_units;
    std::vector<std::string> start_preview_commands;
};

struct FeatureStartRequest {
    std::string feature_name;
    std::vector<std::string> override_args;
    bool restart_if_running = false;
    bool start_with_terminal = false;
};

struct FeatureStartResponse {
    bool success = false;
    std::string message;
    std::string feature_name;
};

struct FeatureStopRequest {
    std::string feature_name;
    bool force = false;
};

struct FeatureStopResponse {
    bool success = false;
    std::string message;
    std::string feature_name;
};

struct RuntimeLogSummary {
    std::string runtime_id;
    std::string feature_name;
    std::string title;
    std::string state;
    uint64_t started_at_ns = 0;
    uint64_t finished_at_ns = 0;
    bool has_exit_code = false;
    int32_t exit_code = 0;
    std::string message;
};

struct RuntimeLogListRequest {
    uint8_t reserved = 0;
};

struct RuntimeLogListResponse {
    bool success = false;
    std::string message;
    std::vector<RuntimeLogSummary> runtimes;
};

struct RuntimeLogReadRequest {
    std::string runtime_id;
    uint64_t cursor = 0;
    uint32_t max_bytes = 0;
};

struct RuntimeLogReadResponse {
    bool success = false;
    std::string message;
    std::string runtime_id;
    std::string chunk;
    uint64_t next_cursor = 0;
    bool truncated = false;
    bool eof = false;
};

/** A currently active topic exposed by an endpoint. */
struct TopicDescriptor {
    std::string name;
    std::string type_name;
    uint32_t publisher_count = 0;
};

struct TopicListRequest {
    uint8_t reserved = 0;
};

struct TopicListResponse {
    bool success = false;
    std::string message;
    std::string revision;
    std::vector<TopicDescriptor> topics;
};

/** Subscribe or unsubscribe one topic for the requesting session. */
struct TopicSubscriptionRequest {
    std::string topic_name;
    bool subscribe = true;
    float max_rate_hz = 0.0F;
    uint32_t max_payload_bytes = 0;
};

struct TopicSubscriptionResponse {
    bool success = false;
    std::string message;
    std::string topic_name;
    bool subscribed = false;
    std::string type_name;
    float max_rate_hz = 0.0F;
    uint32_t max_payload_bytes = 0;
};

enum class ManagedEntityAvailability : uint8_t {
    kUnknown = 0,
    kOnline = 1,
    kDegraded = 2,
    kOffline = 3,
};

/** A logical entity hosted by one physical YunLink endpoint. */
struct ManagedEntityDescriptor {
    std::string entity_uid;
    EndpointIdentity identity;
    std::string display_name;
    std::string hardware_id;
    std::vector<std::string> capabilities;
    ManagedEntityAvailability availability = ManagedEntityAvailability::kUnknown;
};

struct ManagedEntityListRequest {
    /** New clients set this after discovering `managed-entity-attachments-v1`.
     * Old clients leave it false and retain all-entity streaming semantics. */
    bool attachment_aware = false;
};

struct ManagedEntityListResponse {
    bool success = false;
    std::string message;
    std::string endpoint_uid;
    std::string revision;
    EndpointIdentity primary_identity;
    std::vector<ManagedEntityDescriptor> entities;
};

struct ManagedEntityDirectoryChanged {
    std::string endpoint_uid;
    std::string revision;
};

/** Explicit per-session delivery state for a managed logical entity. */
enum class ManagedEntityAttachmentAction : uint8_t {
    kAttach = 1,
    kDetach = 2,
};

struct ManagedEntityAttachmentRequest {
    std::string endpoint_uid;
    std::string directory_revision;
    ManagedEntityAttachmentAction action = ManagedEntityAttachmentAction::kAttach;
    std::vector<std::string> entity_uids;
};

/** One requested entity's outcome.  A response is intentionally partial-success capable. */
struct ManagedEntityAttachmentResult {
    std::string entity_uid;
    bool accepted = false;
    std::string message;
};

struct ManagedEntityAttachmentResponse {
    bool success = false;
    std::string message;
    std::string endpoint_uid;
    std::string directory_revision;
    std::vector<ManagedEntityAttachmentResult> results;
    std::vector<std::string> attached_entity_uids;
};

struct SystemServiceHandle {
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    TargetSelector target;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP
