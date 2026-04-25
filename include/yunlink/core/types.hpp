/**
 * @file include/yunlink/core/types.hpp
 * @brief yunlink 核心类型定义。
 */

#ifndef YUNLINK_CORE_TYPES_HPP
#define YUNLINK_CORE_TYPES_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace yunlink {

using ByteBuffer = std::vector<uint8_t>;

enum class TransportType : uint8_t {
    kTcpServer = 1,
    kTcpClient = 2,
    kUdpUnicast = 3,
    kUdpBroadcast = 4,
    kUdpMulticast = 5,
};

enum class ErrorCode : uint16_t {
    kOk = 0,
    kInvalidArgument,
    kSocketError,
    kBindError,
    kListenError,
    kConnectError,
    kTimeout,
    kEncodeError,
    kDecodeError,
    kChecksumMismatch,
    kInvalidHeader,
    kRuntimeStopped,
    kProtocolMismatch,
    kUnauthorized,
    kRejected,
    kInternal,
};

enum class AgentType : uint8_t {
    kUnknown = 0,
    kGroundStation = 1,
    kUav = 2,
    kUgv = 3,
    kSwarmController = 4,
};

enum class EndpointRole : uint8_t {
    kUnknown = 0,
    kObserver = 1,
    kController = 2,
    kVehicle = 3,
    kRelay = 4,
};

enum class TargetScope : uint8_t {
    kEntity = 1,
    kGroup = 2,
    kBroadcast = 3,
};

enum class MessageFamily : uint8_t {
    kSession = 1,
    kAuthority = 2,
    kCommand = 3,
    kCommandResult = 4,
    kStateSnapshot = 5,
    kStateEvent = 6,
    kBulkChannelDescriptor = 7,
};

enum class QosClass : uint8_t {
    kReliableOrdered = 1,
    kReliableLatest = 2,
    kBestEffort = 3,
    kBulk = 4,
};

enum class SessionState : uint8_t {
    kDiscovered = 1,
    kHandshaking = 2,
    kAuthenticated = 3,
    kNegotiated = 4,
    kActive = 5,
    kDraining = 6,
    kClosed = 7,
    kLost = 8,
    kInvalid = 9,
};

enum class ControlSource : uint8_t {
    kUnknown = 0,
    kGroundStation = 1,
    kRemoteController = 2,
    kTerminal = 3,
    kAutonomy = 4,
};

enum class AuthorityState : uint8_t {
    kObserver = 0,
    kPendingGrant = 1,
    kController = 2,
    kPreempting = 3,
    kRevoked = 4,
    kReleased = 5,
    kRejected = 6,
};

enum class CommandPhase : uint8_t {
    kReceived = 1,
    kAccepted = 2,
    kInProgress = 3,
    kSucceeded = 4,
    kFailed = 5,
    kCancelled = 6,
    kExpired = 7,
};

enum class VehicleEventKind : uint8_t {
    kInfo = 1,
    kTakeoff = 2,
    kLanding = 3,
    kReturnHome = 4,
    kFormationUpdate = 5,
    kFault = 6,
};

enum class BulkStreamType : uint8_t {
    kPointCloud = 1,
    kMapTile = 2,
    kVideo = 3,
};

enum class BulkChannelState : uint8_t {
    kReady = 1,
    kFailed = 2,
    kClosed = 3,
};

struct PeerInfo {
    std::string id;
    std::string ip;
    uint16_t port = 0;
};

struct EndpointIdentity {
    AgentType agent_type = AgentType::kUnknown;
    uint32_t agent_id = 0;
    EndpointRole role = EndpointRole::kUnknown;
    std::vector<uint32_t> group_ids;
};

struct TargetSelector {
    TargetScope scope = TargetScope::kEntity;
    AgentType target_type = AgentType::kUnknown;
    uint32_t group_id = 0;
    std::vector<uint32_t> target_ids;

    static TargetSelector for_entity(AgentType type, uint32_t id) {
        TargetSelector out;
        out.scope = TargetScope::kEntity;
        out.target_type = type;
        out.target_ids.push_back(id);
        return out;
    }

    static TargetSelector for_group(AgentType type, uint32_t group) {
        TargetSelector out;
        out.scope = TargetScope::kGroup;
        out.target_type = type;
        out.group_id = group;
        return out;
    }

    static TargetSelector broadcast(AgentType type) {
        TargetSelector out;
        out.scope = TargetScope::kBroadcast;
        out.target_type = type;
        return out;
    }

    bool matches(const EndpointIdentity& self) const {
        if (scope == TargetScope::kBroadcast) {
            return target_type == AgentType::kUnknown || target_type == self.agent_type;
        }
        if (scope == TargetScope::kGroup) {
            if (target_type != AgentType::kUnknown && target_type != self.agent_type) {
                return false;
            }
            for (uint32_t id : self.group_ids) {
                if (id == group_id) {
                    return true;
                }
            }
            return false;
        }
        if (target_type != AgentType::kUnknown && target_type != self.agent_type) {
            return false;
        }
        for (uint32_t id : target_ids) {
            if (id == self.agent_id) {
                return true;
            }
        }
        return false;
    }
};

struct SecurityContext {
    uint32_t key_epoch = 0;
    ByteBuffer auth_tag;
};

struct SecureEnvelope {
    uint8_t protocol_major = 1;
    uint8_t header_version = 1;
    uint16_t flags = 0;
    uint16_t header_len = 0;
    uint32_t payload_len = 0;
    QosClass qos_class = QosClass::kReliableOrdered;
    MessageFamily message_family = MessageFamily::kCommand;
    uint16_t message_type = 0;
    uint16_t schema_version = 1;
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    EndpointIdentity source;
    TargetSelector target;
    uint64_t created_at_ms = 0;
    uint32_t ttl_ms = 0;
    ByteBuffer payload;
    SecurityContext security;
    uint32_t checksum = 0;
};

struct DecodeResult {
    ErrorCode code = ErrorCode::kOk;
    size_t consumed = 0;
    SecureEnvelope envelope;

    bool ok() const {
        return code == ErrorCode::kOk;
    }
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_TYPES_HPP
