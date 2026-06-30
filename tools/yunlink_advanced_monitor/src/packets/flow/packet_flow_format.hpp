#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_FORMAT_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_FORMAT_HPP

#include <string>

#include "packets/flow/packet_flow_model.hpp"

std::string packet_flow_stage_title(PacketFlowStage stage);
std::string packet_flow_stage_short(PacketFlowStage stage);
std::string packet_flow_mode_label(PacketFlowMode mode);
std::string packet_flow_journey_detail(const PacketFlowSnapshot& snapshot);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_FORMAT_HPP
