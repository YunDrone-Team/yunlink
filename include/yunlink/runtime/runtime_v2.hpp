/**
 * @file include/yunlink/runtime/runtime_v2.hpp
 * @brief Provider-neutral YunLink v2 runtime facade.
 */

#ifndef YUNLINK_RUNTIME_RUNTIME_V2_HPP
#define YUNLINK_RUNTIME_RUNTIME_V2_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "yunlink/core/wire_v2.hpp"

namespace yunlink::v2 {

enum class SessionOperation : uint8_t {
    kHello = 1,
    kAuthenticate = 2,
    kProfiles = 3,
    kReady = 4,
};

enum class AuthorityOperation : uint8_t {
    kClaim = 1,
    kRenew = 2,
    kRelease = 3,
    kStatus = 4,
};

enum class DirectoryOperation : uint8_t {
    kListRequest = 1,
    kListResponse = 2,
    kChanged = 3,
    kAttachRequest = 4,
    kAttachResponse = 5,
    kDetachRequest = 6,
    kDetachResponse = 7,
};

enum class StreamOperation : uint8_t {
    kCatalogRequest = 1,
    kCatalogResponse = 2,
    kSubscribe = 3,
    kSample = 4,
    kUnsubscribe = 5,
    kSubscriptionStatus = 6,
};

enum class ActionOperation : uint8_t {
    kGoal = 1,
    kUpdate = 2,
    kCancel = 3,
};

enum class RpcOperation : uint8_t {
    kRequest = 1,
    kResponse = 2,
};

enum class ConfigurationOperation : uint8_t {
    kListRequest = 1,
    kListResponse = 2,
    kDescribeRequest = 3,
    kDescribeResponse = 4,
    kGetRequest = 5,
    kGetResponse = 6,
    kPatchRequest = 7,
    kPatchResponse = 8,
    kApplyRequest = 9,
    kApplyResponse = 10,
};

enum class LogOperation : uint8_t {
    kListRequest = 1,
    kListResponse = 2,
    kReadRequest = 3,
    kReadResponse = 4,
};

enum class BulkOperation : uint8_t {
    kOpen = 1,
    kChunk = 2,
    kClose = 3,
    kStatus = 4,
};

struct QosChannelPolicy {
    MessageFamily family = MessageFamily::kStream;
    std::string profile_id;
    std::string type_name;
    QosClass qos_class = QosClass::kReliableLatest;
};

struct RuntimeConfig {
    std::string endpoint_uid = "yunlink-endpoint";
    std::string display_name = "yunlink-endpoint";
    std::vector<EntityDescriptor> entities;
    std::vector<std::string> capabilities;
    std::vector<ProfileDescriptor> profiles;
    std::vector<ProfileDescriptor> required_profiles;
    std::vector<std::string> group_uids;
    std::vector<QosChannelPolicy> qos_policies;
    std::string shared_secret = "yunlink-default-secret";
    uint16_t tcp_listen_port = 9696;
    int connect_timeout_ms = 5000;
    int io_poll_interval_ms = 5;
    size_t max_buffer_bytes_per_peer = 16U * 1024U * 1024U + 65536U;
};

struct Peer {
    std::string id;
    std::string ip;
    uint16_t port = 0;
};

enum class SessionState : uint8_t {
    kHandshaking = 1,
    kAuthenticated = 2,
    kNegotiated = 3,
    kActive = 4,
    kInvalid = 5,
    kLost = 6,
};

struct SessionInfo {
    uint64_t session_id = 0;
    std::string peer_id;
    std::string remote_endpoint_uid;
    SessionState state = SessionState::kHandshaking;
    bool authenticated = false;
    std::map<std::string, ProfileDescriptor> negotiated_profiles;
    std::vector<std::string> rejected_profiles;

    bool has_profile(const std::string& profile_id, uint16_t major) const;
};

enum class RuntimeEventKind : uint8_t {
    kEnvelope = 1,
    kLink = 2,
    kSession = 3,
    kError = 4,
};

struct RuntimeEvent {
    RuntimeEventKind kind = RuntimeEventKind::kEnvelope;
    Peer peer;
    Envelope envelope;
    SessionInfo session;
    ErrorCode error = ErrorCode::kOk;
    std::string message;
    bool link_up = false;
};

struct MessageHandle {
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
};

class Runtime {
  public:
    struct Impl;
    using EventHandler = std::function<void(const RuntimeEvent&)>;

    Runtime();
    ~Runtime();
    Runtime(const Runtime&) = delete;
    Runtime& operator=(const Runtime&) = delete;

    ErrorCode start(const RuntimeConfig& config);
    void stop();
    bool running() const;

    ErrorCode connect_peer(const std::string& ip, uint16_t port, Peer* out = nullptr);
    void close_peer(const std::string& peer_id);
    uint64_t open_session(const std::string& peer_id);

    ErrorCode publish(const std::string& peer_id,
                      uint64_t session_id,
                      MessageFamily family,
                      uint8_t operation,
                      const TargetSelector& target,
                      const TypeRef& type,
                      const Bytes& payload,
                      MessageHandle* out = nullptr,
                      uint64_t correlation_id = 0,
                      uint32_t ttl_ms = 0,
                      QosClass qos = QosClass::kReliableOrdered,
                      const std::string& source_entity_uid = {});
    ErrorCode send(const std::string& peer_id, const Envelope& envelope);

    size_t subscribe(EventHandler handler);
    void unsubscribe(size_t token);
    std::vector<SessionInfo> sessions() const;
    bool session(const std::string& peer_id, uint64_t session_id, SessionInfo* out) const;

  private:
    std::unique_ptr<Impl> impl_;
};

}  // namespace yunlink::v2

#endif  // YUNLINK_RUNTIME_RUNTIME_V2_HPP
