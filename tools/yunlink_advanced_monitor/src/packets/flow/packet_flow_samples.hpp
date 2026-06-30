#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_SAMPLES_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_SAMPLES_HPP

#include <cstddef>
#include <cstdint>

#include "packets/flow/packet_flow_model.hpp"

PacketFlowSnapshot packet_flow_takeoff_demo_snapshot(size_t active_step, uint64_t now_ms);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_SAMPLES_HPP
