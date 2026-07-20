/**
 * @file include/yunlink/core/semantic/message_ids.hpp
 * @brief Semantic protocol message identifiers.
 */

#ifndef YUNLINK_CORE_SEMANTIC_MESSAGE_IDS_HPP
#define YUNLINK_CORE_SEMANTIC_MESSAGE_IDS_HPP

#include <cstdint>

namespace yunlink {

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
    kUavControl = 8,
};

enum class StateSnapshotType : uint16_t {
    kVehicleCore = 1,
    kPx4State = 2,
    kOdomStatus = 3,
    kUavControlFsmState = 4,
    kUavControllerState = 5,
    kGimbalParams = 6,
    kLocalOdom = 7,
    kUavControlCmd = 8,
    kUavControlState = 9,
    kOdomState = 10,
    kSunrayRuntimeDiagnostic = 11,
    kCommandExecutionStatus = 12,
    kHostSystem = 13,
    kTopicSample = 14,
};

enum class StateEventType : uint16_t {
    kVehicleEvent = 1,
};

enum class BulkDescriptorType : uint16_t {
    kDescriptor = 1,
};

enum class SystemServiceType : uint16_t {
    kFeatureListRequest = 1,
    kFeatureListResponse = 2,
    kFeatureGetRequest = 3,
    kFeatureGetResponse = 4,
    kFeatureStartRequest = 5,
    kFeatureStartResponse = 6,
    kFeatureStopRequest = 7,
    kFeatureStopResponse = 8,
    kRuntimeLogListRequest = 9,
    kRuntimeLogListResponse = 10,
    kRuntimeLogReadRequest = 11,
    kRuntimeLogReadResponse = 12,
    kTopicListRequest = 13,
    kTopicListResponse = 14,
    kTopicSubscriptionRequest = 15,
    kTopicSubscriptionResponse = 16,
    kManagedEntityListRequest = 17,
    kManagedEntityListResponse = 18,
    kManagedEntityDirectoryChanged = 19,
};

enum class ConfigurationServiceType : uint16_t {
    kResourceListRequest = 1,
    kResourceListResponse = 2,
    kResourceDescribeRequest = 3,
    kResourceDescribeResponse = 4,
    kResourceGetRequest = 5,
    kResourceGetResponse = 6,
    kResourcePatchRequest = 7,
    kResourcePatchResponse = 8,
    kResourceApplyRequest = 9,
    kResourceApplyResponse = 10,
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
    kUavControl = 8,
};

enum class AuthorityAction : uint8_t {
    kClaim = 1,
    kRenew = 2,
    kRelease = 3,
    kPreempt = 4,
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_MESSAGE_IDS_HPP
