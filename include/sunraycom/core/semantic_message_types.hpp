/**
 * @file include/sunraycom/core/semantic_message_types.hpp
 * @brief 新协议语义消息模型定义。
 */

#ifndef SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TYPES_HPP
#define SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TYPES_HPP

#include <string>
#include <vector>

#include "sunraycom/core/types.hpp"

namespace sunraycom {

enum class SessionMessageType : uint16_t {
    kHello = 1,
    kAuthenticate = 2,
    kCapabilities = 3,
    kReady = 4,
};

enum class AuthorityMessageType : uint16_t {
    kRequest = 1,
    kStatus = 2,
};

enum class CommandType : uint16_t {
    kTakeoff = 1,
    kLand = 2,
    kReturn = 3,
    kGoto = 4,
    kVelocitySetpoint = 5,
    kTrajectoryChunk = 6,
    kFormationTask = 7,
};

enum class StateSnapshotType : uint16_t {
    kVehicleCore = 1,
};

enum class StateEventType : uint16_t {
    kVehicleEvent = 1,
};

enum class BulkDescriptorType : uint16_t {
    kDescriptor = 1,
};

enum class CommandKind : uint16_t {
    kUnknown = 0,
    kTakeoff = 1,
    kLand = 2,
    kReturn = 3,
    kGoto = 4,
    kVelocitySetpoint = 5,
    kTrajectoryChunk = 6,
    kFormationTask = 7,
};

enum class AuthorityAction : uint8_t {
    kClaim = 1,
    kRenew = 2,
    kRelease = 3,
    kPreempt = 4,
};

struct SessionHello {
    std::string node_name;
    uint32_t capability_flags = 0;
};

struct SessionAuthenticate {
    std::string shared_secret;
};

struct SessionCapabilities {
    uint32_t capability_flags = 0;
};

struct SessionReady {
    uint8_t accepted_protocol_major = 1;
};

struct AuthorityRequest {
    AuthorityAction action = AuthorityAction::kClaim;
    ControlSource source = ControlSource::kUnknown;
    uint32_t lease_ttl_ms = 0;
    bool allow_preempt = false;
};

struct AuthorityStatus {
    AuthorityState state = AuthorityState::kObserver;
    uint64_t session_id = 0;
    uint32_t lease_ttl_ms = 0;
    uint16_t reason_code = 0;
    std::string detail;
};

struct TakeoffCommand {
    float relative_height_m = 0.0F;
    float max_velocity_mps = 0.0F;
};

struct LandCommand {
    float max_velocity_mps = 0.0F;
};

struct ReturnCommand {
    float loiter_before_return_s = 0.0F;
};

struct GotoCommand {
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float yaw_rad = 0.0F;
};

struct VelocitySetpointCommand {
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float yaw_rate_radps = 0.0F;
    bool body_frame = false;
};

struct TrajectoryPoint {
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float yaw_rad = 0.0F;
    uint32_t dt_ms = 0;
};

struct TrajectoryChunkCommand {
    uint32_t chunk_index = 0;
    bool final_chunk = false;
    std::vector<TrajectoryPoint> points;
};

struct FormationTaskCommand {
    uint32_t group_id = 0;
    uint8_t formation_shape = 0;
    float spacing_m = 0.0F;
    std::string label;
};

struct CommandResult {
    CommandKind command_kind = CommandKind::kUnknown;
    CommandPhase phase = CommandPhase::kReceived;
    uint16_t result_code = 0;
    uint8_t progress_percent = 0;
    std::string detail;
};

struct VehicleCoreState {
    bool armed = false;
    uint8_t nav_mode = 0;
    float x_m = 0.0F;
    float y_m = 0.0F;
    float z_m = 0.0F;
    float vx_mps = 0.0F;
    float vy_mps = 0.0F;
    float vz_mps = 0.0F;
    float battery_percent = 0.0F;
};

struct VehicleEvent {
    VehicleEventKind kind = VehicleEventKind::kInfo;
    uint8_t severity = 0;
    std::string detail;
};

struct BulkChannelDescriptor {
    BulkStreamType stream_type = BulkStreamType::kPointCloud;
    std::string uri;
    uint32_t mtu_bytes = 0;
    bool reliable = false;
};

struct SessionDescriptor {
    uint64_t session_id = 0;
    SessionState state = SessionState::kDiscovered;
    EndpointIdentity remote_identity;
    PeerInfo peer;
    uint32_t capability_flags = 0;
    std::string node_name;
};

struct AuthorityLease {
    AuthorityState state = AuthorityState::kObserver;
    uint64_t session_id = 0;
    TargetSelector target;
    ControlSource source = ControlSource::kUnknown;
    uint32_t lease_ttl_ms = 0;
    uint64_t expires_at_ms = 0;
    PeerInfo peer;
};

template <typename T> struct TypedMessage {
    SecureEnvelope envelope;
    T payload;
};

using CommandResultView = TypedMessage<CommandResult>;

struct CommandHandle {
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    TargetSelector target;
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TYPES_HPP
