#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_MODEL_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <yunlink/yunlink.hpp>

enum class PacketFlowStage {
    kSemanticPayload,
    kSecureEnvelope,
    kEncode,
    kTransportSend,
    kTransportReceive,
    kDecode,
    kRuntimeDispatch,
    kSemanticSubscriber,
};

enum class PacketFlowMode {
    kLive,
    kSelected,
    kDemo,
};

struct PacketFlowStep {
    PacketFlowStage stage{PacketFlowStage::kSemanticPayload};
    uint64_t observed_at_ms{0};
    uint64_t trace_id{0};
    std::string title;
    std::string subtitle;
    std::string detail;
    bool observed{false};
    bool active{false};
    bool failed{false};
};

struct PacketFlowJourney {
    std::string title;
    std::string subtitle;
    std::vector<PacketFlowStep> steps;
    uint64_t newest_trace_id{0};
    uint64_t newest_at_ms{0};
};

struct PacketFlowSnapshot {
    PacketFlowMode mode{PacketFlowMode::kLive};
    std::vector<PacketFlowJourney> journeys;
    uint64_t newest_trace_id{0};
    uint64_t generated_at_ms{0};
};

PacketFlowJourney packet_flow_empty_journey(const std::string& title,
                                            const std::string& subtitle);
PacketFlowSnapshot packet_flow_live_snapshot(
    const std::vector<yunlink::PacketTraceRecord>& records,
    uint64_t now_ms,
    uint64_t window_ms,
    size_t max_journeys);
PacketFlowSnapshot packet_flow_selected_snapshot(
    const std::vector<yunlink::PacketTraceRecord>& records,
    uint64_t selected_trace_id,
    uint64_t now_ms);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_FLOW_PACKET_FLOW_MODEL_HPP
