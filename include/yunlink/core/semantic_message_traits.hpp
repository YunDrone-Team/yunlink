/**
 * @file include/yunlink/core/semantic_message_traits.hpp
 * @brief 新协议语义消息 traits 定义。
 */

#ifndef YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
#define YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP

#include "yunlink/core/semantic_message_types.hpp"

namespace yunlink {

template <typename T> struct MessageTraits;

#define YUNLINK_DEFINE_TRAITS(TYPE, FAMILY, TYPE_ID)                                               \
    template <> struct MessageTraits<TYPE> {                                                       \
        static constexpr MessageFamily kFamily = FAMILY;                                           \
        static constexpr uint16_t kMessageType = TYPE_ID;                                          \
        static constexpr uint16_t kSchemaVersion = 1;                                              \
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
YUNLINK_DEFINE_TRAITS(VehicleCoreState, MessageFamily::kStateSnapshot, 1);
YUNLINK_DEFINE_TRAITS(Px4StateSnapshot, MessageFamily::kStateSnapshot, 2);
YUNLINK_DEFINE_TRAITS(OdomStatusSnapshot, MessageFamily::kStateSnapshot, 3);
YUNLINK_DEFINE_TRAITS(UavControlFsmStateSnapshot, MessageFamily::kStateSnapshot, 4);
YUNLINK_DEFINE_TRAITS(UavControllerStateSnapshot, MessageFamily::kStateSnapshot, 5);
YUNLINK_DEFINE_TRAITS(GimbalParamsSnapshot, MessageFamily::kStateSnapshot, 6);
YUNLINK_DEFINE_TRAITS(VehicleEvent, MessageFamily::kStateEvent, 1);
YUNLINK_DEFINE_TRAITS(BulkChannelDescriptor, MessageFamily::kBulkChannelDescriptor, 1);

#undef YUNLINK_DEFINE_TRAITS

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
