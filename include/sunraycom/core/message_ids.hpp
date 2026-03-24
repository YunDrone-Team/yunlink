/**
 * @file include/sunraycom/core/message_ids.hpp
 * @brief 协议消息序号定义。
 */

#ifndef SUNRAYCOM_CORE_MESSAGE_IDS_HPP
#define SUNRAYCOM_CORE_MESSAGE_IDS_HPP

#include <cstdint>

namespace sunraycom {

/**
 * @brief 协议消息类型枚举。
 */
enum class MessageId : uint8_t {
    HeartbeatMessageID = 1,
    UAVStateMessageID = 2,
    PX4StateMessageID = 3,
    PX4ParameterMessageID = 4,
    UGVStateMessageID = 20,
    NodeMessageID = 30,
    AgentComputerStatusMessageID = 31,
    FACMapDataMessageID = 32,
    FACCompetitionStateMessageID = 33,
    WaypointStateMessageID = 34,
    FormationMessageID = 40,
    GroundFormationMessageID = 41,
    PointCloudDataMessageID = 50,
    UAVControlCMDMessageID = 102,
    UAVSetupMessageID = 103,
    WaypointMessageID = 104,
    ViobotSwitchMessageID = 105,
    RTKOriginMessageID = 106,
    UGVControlCMDMessageID = 120,
    SearchMessageID = 200,
    ACKMessageID = 201,
    DemoMessageID = 202,
    ScriptMessageID = 203,
    GoalMessageID = 204,
    QRCodeCoordMessageID = 205,
    PointCloudDataSwitchMessageID = 206,
};

}  // namespace sunraycom

#endif  // SUNRAYCOM_CORE_MESSAGE_IDS_HPP
