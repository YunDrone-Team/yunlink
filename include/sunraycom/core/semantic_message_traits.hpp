/**
 * @file include/sunraycom/core/semantic_message_traits.hpp
 * @brief 新协议语义消息 traits 定义。
 */

#ifndef SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
#define SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TRAITS_HPP

#include "sunraycom/core/semantic_message_types.hpp"

namespace sunraycom {

template <typename T> struct MessageTraits;

#define SUNRAYCOM_DEFINE_TRAITS(TYPE, FAMILY, TYPE_ID)                                             \
    template <> struct MessageTraits<TYPE> {                                                       \
        static constexpr MessageFamily kFamily = FAMILY;                                           \
        static constexpr uint16_t kMessageType = TYPE_ID;                                          \
        static constexpr uint16_t kSchemaVersion = 1;                                              \
    }

SUNRAYCOM_DEFINE_TRAITS(SessionHello, MessageFamily::kSession, 1);
SUNRAYCOM_DEFINE_TRAITS(SessionAuthenticate, MessageFamily::kSession, 2);
SUNRAYCOM_DEFINE_TRAITS(SessionCapabilities, MessageFamily::kSession, 3);
SUNRAYCOM_DEFINE_TRAITS(SessionReady, MessageFamily::kSession, 4);
SUNRAYCOM_DEFINE_TRAITS(AuthorityRequest, MessageFamily::kAuthority, 1);
SUNRAYCOM_DEFINE_TRAITS(AuthorityStatus, MessageFamily::kAuthority, 2);
SUNRAYCOM_DEFINE_TRAITS(TakeoffCommand, MessageFamily::kCommand, 1);
SUNRAYCOM_DEFINE_TRAITS(LandCommand, MessageFamily::kCommand, 2);
SUNRAYCOM_DEFINE_TRAITS(ReturnCommand, MessageFamily::kCommand, 3);
SUNRAYCOM_DEFINE_TRAITS(GotoCommand, MessageFamily::kCommand, 4);
SUNRAYCOM_DEFINE_TRAITS(VelocitySetpointCommand, MessageFamily::kCommand, 5);
SUNRAYCOM_DEFINE_TRAITS(TrajectoryChunkCommand, MessageFamily::kCommand, 6);
SUNRAYCOM_DEFINE_TRAITS(FormationTaskCommand, MessageFamily::kCommand, 7);
SUNRAYCOM_DEFINE_TRAITS(CommandResult, MessageFamily::kCommandResult, 1);
SUNRAYCOM_DEFINE_TRAITS(VehicleCoreState, MessageFamily::kStateSnapshot, 1);
SUNRAYCOM_DEFINE_TRAITS(Px4StateSnapshot, MessageFamily::kStateSnapshot, 2);
SUNRAYCOM_DEFINE_TRAITS(OdomStatusSnapshot, MessageFamily::kStateSnapshot, 3);
SUNRAYCOM_DEFINE_TRAITS(UavControlFsmStateSnapshot, MessageFamily::kStateSnapshot, 4);
SUNRAYCOM_DEFINE_TRAITS(UavControllerStateSnapshot, MessageFamily::kStateSnapshot, 5);
SUNRAYCOM_DEFINE_TRAITS(GimbalParamsSnapshot, MessageFamily::kStateSnapshot, 6);
SUNRAYCOM_DEFINE_TRAITS(VehicleEvent, MessageFamily::kStateEvent, 1);
SUNRAYCOM_DEFINE_TRAITS(BulkChannelDescriptor, MessageFamily::kBulkChannelDescriptor, 1);

#undef SUNRAYCOM_DEFINE_TRAITS

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_SEMANTIC_MESSAGE_TRAITS_HPP
