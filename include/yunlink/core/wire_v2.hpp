/**
 * @file include/yunlink/core/wire_v2.hpp
 * @brief YunLink Wire v2 provider-neutral protocol contract.
 */

#ifndef YUNLINK_CORE_WIRE_V2_HPP
#define YUNLINK_CORE_WIRE_V2_HPP

#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace yunlink::v2 {

using Bytes = std::vector<uint8_t>;

constexpr uint8_t kProtocolMajor = 2;
constexpr uint8_t kHeaderVersion = 2;
constexpr uint16_t kSchemaVersion = 2;
constexpr size_t kMaxUidBytes = 128;
constexpr size_t kMaxTypeNameBytes = 256;
constexpr size_t kMaxProfileIdBytes = 128;
constexpr size_t kMaxPayloadBytes = 16U * 1024U * 1024U;

enum class ErrorCode : uint16_t {
    kOk = 0,
    kInvalidArgument,
    kEncodeError,
    kDecodeError,
    kChecksumMismatch,
    kInvalidHeader,
    kProtocolMismatch,
    kTimeout,
    kUnauthorized,
    kRejected,
    kNotFound,
    kConflict,
    kUnsupported,
    kInternal,
};

enum class MessageFamily : uint8_t {
    kSession = 1,
    kAuthority = 2,
    kEntityDirectory = 3,
    kStream = 4,
    kAction = 5,
    kRpc = 6,
    kConfiguration = 7,
    kLog = 8,
    kBulk = 9,
};

enum class QosClass : uint8_t {
    kReliableOrdered = 1,
    kReliableLatest = 2,
    kBestEffort = 3,
    kBulk = 4,
};

enum class TargetScope : uint8_t {
    kEndpoint = 1,
    kEntity = 2,
    kGroup = 3,
    kBroadcast = 4,
};

enum class Availability : uint8_t {
    kUnknown = 0,
    kOnline = 1,
    kDegraded = 2,
    kOffline = 3,
};

enum class ActionPhase : uint8_t {
    kReceived = 1,
    kAccepted = 2,
    kRunning = 3,
    kSucceeded = 4,
    kFailed = 5,
    kCancelled = 6,
    kExpired = 7,
};

struct TypeRef {
    std::string profile_id;
    uint16_t major = 0;
    uint16_t minor = 0;
    std::string type_name;

    bool is_core() const {
        return profile_id == "yunlink.core";
    }
};

struct ProfileDescriptor {
    std::string profile_id;
    uint16_t major = 0;
    uint16_t minor = 0;
    std::string schema_digest;
};

struct SourceIdentity {
    std::string endpoint_uid;
    std::string entity_uid;
};

struct TargetSelector {
    TargetScope scope = TargetScope::kBroadcast;
    std::vector<std::string> uids;

    static TargetSelector endpoint(std::string uid);
    static TargetSelector entity(std::string uid);
    static TargetSelector group(std::string uid);
    static TargetSelector broadcast();
    bool matches(const std::string& endpoint_uid,
                 const std::vector<std::string>& entity_uids,
                 const std::vector<std::string>& group_uids) const;
};

struct SecurityContext {
    uint32_t key_epoch = 0;
    Bytes auth_tag;
};

struct Envelope {
    uint8_t protocol_major = kProtocolMajor;
    uint8_t header_version = kHeaderVersion;
    uint16_t flags = 0;
    uint16_t header_len = 0;
    uint32_t payload_len = 0;
    QosClass qos_class = QosClass::kReliableOrdered;
    MessageFamily family = MessageFamily::kSession;
    uint8_t operation = 0;
    uint16_t schema_version = kSchemaVersion;
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    SourceIdentity source;
    TargetSelector target;
    TypeRef type;
    uint64_t created_at_ms = 0;
    uint32_t ttl_ms = 0;
    Bytes payload;
    SecurityContext security;
    uint32_t checksum = 0;
};

struct DecodeResult {
    ErrorCode code = ErrorCode::kOk;
    size_t consumed = 0;
    Envelope envelope;

    bool ok() const {
        return code == ErrorCode::kOk;
    }
};

struct EntityDescriptor {
    std::string entity_uid;
    std::string kind;
    std::string display_name;
    std::string hardware_id;
    std::map<std::string, std::string> attributes;
    std::vector<std::string> capabilities;
    Availability availability = Availability::kUnknown;
};

bool valid_uid(const std::string& value);
bool valid_profile_id(const std::string& value);
bool valid_type_ref(const TypeRef& value);

}  // namespace yunlink::v2

#endif  // YUNLINK_CORE_WIRE_V2_HPP
