/**
 * @file include/yunlink/core/semantic/message_traits.hpp
 * @brief Semantic message trait mapping.
 */

#ifndef YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
#define YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP

#include "yunlink/core/semantic/message_types.hpp"

namespace yunlink {

template <typename T> struct MessageTraits;

#define YUNLINK_DEFINE_TRAITS(TYPE, FAMILY, TYPE_ID)                                               \
    template <> struct MessageTraits<TYPE> {                                                       \
        static constexpr MessageFamily kFamily = FAMILY;                                           \
        static constexpr uint16_t kMessageType = TYPE_ID;                                          \
        static constexpr uint16_t kSchemaVersion = kCurrentSchemaVersion;                          \
    }

YUNLINK_DEFINE_TRAITS(SessionHello, MessageFamily::kSession, 1);
YUNLINK_DEFINE_TRAITS(SessionAuthenticate, MessageFamily::kSession, 2);
YUNLINK_DEFINE_TRAITS(SessionCapabilities, MessageFamily::kSession, 3);
YUNLINK_DEFINE_TRAITS(SessionReady, MessageFamily::kSession, 4);
YUNLINK_DEFINE_TRAITS(AuthorityRequest, MessageFamily::kAuthority, 1);
YUNLINK_DEFINE_TRAITS(AuthorityStatus, MessageFamily::kAuthority, 2);
YUNLINK_DEFINE_TRAITS(TakeoffCommand, MessageFamily::kCommand, 1);
YUNLINK_DEFINE_TRAITS(LandCommand, MessageFamily::kCommand, 2);
YUNLINK_DEFINE_TRAITS(ReturnCommand, MessageFamily::kCommand, 3);
YUNLINK_DEFINE_TRAITS(GotoCommand, MessageFamily::kCommand, 4);
YUNLINK_DEFINE_TRAITS(VelocitySetpointCommand, MessageFamily::kCommand, 5);
YUNLINK_DEFINE_TRAITS(TrajectoryChunkCommand, MessageFamily::kCommand, 6);
YUNLINK_DEFINE_TRAITS(FormationTaskCommand, MessageFamily::kCommand, 7);
YUNLINK_DEFINE_TRAITS(CommandResult, MessageFamily::kCommandResult, 1);
YUNLINK_DEFINE_TRAITS(FeatureListRequest, MessageFamily::kSystemService, 1);
YUNLINK_DEFINE_TRAITS(FeatureListResponse, MessageFamily::kSystemService, 2);
YUNLINK_DEFINE_TRAITS(FeatureGetRequest, MessageFamily::kSystemService, 3);
YUNLINK_DEFINE_TRAITS(FeatureGetResponse, MessageFamily::kSystemService, 4);
YUNLINK_DEFINE_TRAITS(FeatureStartRequest, MessageFamily::kSystemService, 5);
YUNLINK_DEFINE_TRAITS(FeatureStartResponse, MessageFamily::kSystemService, 6);
YUNLINK_DEFINE_TRAITS(FeatureStopRequest, MessageFamily::kSystemService, 7);
YUNLINK_DEFINE_TRAITS(FeatureStopResponse, MessageFamily::kSystemService, 8);
YUNLINK_DEFINE_TRAITS(RuntimeLogListRequest, MessageFamily::kSystemService, 9);
YUNLINK_DEFINE_TRAITS(RuntimeLogListResponse, MessageFamily::kSystemService, 10);
YUNLINK_DEFINE_TRAITS(RuntimeLogReadRequest, MessageFamily::kSystemService, 11);
YUNLINK_DEFINE_TRAITS(RuntimeLogReadResponse, MessageFamily::kSystemService, 12);
YUNLINK_DEFINE_TRAITS(ConfigResourceListRequest, MessageFamily::kConfigurationService, 1);
YUNLINK_DEFINE_TRAITS(ConfigResourceListResponse, MessageFamily::kConfigurationService, 2);
YUNLINK_DEFINE_TRAITS(ConfigResourceDescribeRequest, MessageFamily::kConfigurationService, 3);
YUNLINK_DEFINE_TRAITS(ConfigResourceDescribeResponse, MessageFamily::kConfigurationService, 4);
YUNLINK_DEFINE_TRAITS(ConfigResourceGetRequest, MessageFamily::kConfigurationService, 5);
YUNLINK_DEFINE_TRAITS(ConfigResourceGetResponse, MessageFamily::kConfigurationService, 6);
YUNLINK_DEFINE_TRAITS(ConfigResourcePatchRequest, MessageFamily::kConfigurationService, 7);
YUNLINK_DEFINE_TRAITS(ConfigResourcePatchResponse, MessageFamily::kConfigurationService, 8);
YUNLINK_DEFINE_TRAITS(ConfigResourceApplyRequest, MessageFamily::kConfigurationService, 9);
YUNLINK_DEFINE_TRAITS(ConfigResourceApplyResponse, MessageFamily::kConfigurationService, 10);
YUNLINK_DEFINE_TRAITS(VehicleCoreState, MessageFamily::kStateSnapshot, 1);
YUNLINK_DEFINE_TRAITS(Px4StateSnapshot, MessageFamily::kStateSnapshot, 2);
YUNLINK_DEFINE_TRAITS(OdomStatusSnapshot, MessageFamily::kStateSnapshot, 3);
YUNLINK_DEFINE_TRAITS(UavControlFsmStateSnapshot, MessageFamily::kStateSnapshot, 4);
YUNLINK_DEFINE_TRAITS(UavControllerStateSnapshot, MessageFamily::kStateSnapshot, 5);
YUNLINK_DEFINE_TRAITS(GimbalParamsSnapshot, MessageFamily::kStateSnapshot, 6);
YUNLINK_DEFINE_TRAITS(LocalOdomSnapshot, MessageFamily::kStateSnapshot, 7);
YUNLINK_DEFINE_TRAITS(UavControlCmdSnapshot, MessageFamily::kStateSnapshot, 8);
YUNLINK_DEFINE_TRAITS(UavControlStateSnapshot, MessageFamily::kStateSnapshot, 9);
YUNLINK_DEFINE_TRAITS(OdomStateSnapshot, MessageFamily::kStateSnapshot, 10);
YUNLINK_DEFINE_TRAITS(SunrayRuntimeDiagnosticSnapshot, MessageFamily::kStateSnapshot, 11);
YUNLINK_DEFINE_TRAITS(CommandExecutionStatusSnapshot, MessageFamily::kStateSnapshot, 12);
YUNLINK_DEFINE_TRAITS(VehicleEvent, MessageFamily::kStateEvent, 1);
YUNLINK_DEFINE_TRAITS(BulkChannelDescriptor, MessageFamily::kBulkChannelDescriptor, 1);

#undef YUNLINK_DEFINE_TRAITS

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
