#include "packets/flow/packet_flow_format.hpp"

#include <sstream>

namespace {

std::string bool_label(bool value) {
    return value ? "yes" : "no";
}

}  // namespace

std::string packet_flow_stage_title(PacketFlowStage stage) {
    switch (stage) {
    case PacketFlowStage::kSemanticPayload:
        return "Semantic Payload";
    case PacketFlowStage::kSecureEnvelope:
        return "SecureEnvelope";
    case PacketFlowStage::kEncode:
        return "Encode";
    case PacketFlowStage::kTransportSend:
        return "Transport Send";
    case PacketFlowStage::kTransportReceive:
        return "Transport Receive";
    case PacketFlowStage::kDecode:
        return "Decode";
    case PacketFlowStage::kRuntimeDispatch:
        return "Runtime Dispatch";
    case PacketFlowStage::kSemanticSubscriber:
        return "Semantic Subscriber";
    }
    return "Stage";
}

std::string packet_flow_stage_short(PacketFlowStage stage) {
    switch (stage) {
    case PacketFlowStage::kSemanticPayload:
        return "Payload";
    case PacketFlowStage::kSecureEnvelope:
        return "Envelope";
    case PacketFlowStage::kEncode:
        return "Encode";
    case PacketFlowStage::kTransportSend:
        return "Send";
    case PacketFlowStage::kTransportReceive:
        return "Receive";
    case PacketFlowStage::kDecode:
        return "Decode";
    case PacketFlowStage::kRuntimeDispatch:
        return "Dispatch";
    case PacketFlowStage::kSemanticSubscriber:
        return "Subscriber";
    }
    return "Stage";
}

std::string packet_flow_mode_label(PacketFlowMode mode) {
    switch (mode) {
    case PacketFlowMode::kLive:
        return "Live";
    case PacketFlowMode::kSelected:
        return "Selected";
    case PacketFlowMode::kDemo:
        return "Demo";
    }
    return "Flow";
}

std::string packet_flow_journey_detail(const PacketFlowSnapshot& snapshot) {
    std::ostringstream ss;
    ss << "mode: " << packet_flow_mode_label(snapshot.mode) << "\n";
    if (snapshot.journeys.empty()) {
        ss << "journey: none\n";
        return ss.str();
    }
    const auto& journey = snapshot.journeys.front();
    ss << "journey: " << journey.title << "\n";
    ss << "path: " << journey.subtitle << "\n";
    ss << "newest_trace_id: " << journey.newest_trace_id << "\n\n";
    for (const auto& step : journey.steps) {
        ss << packet_flow_stage_short(step.stage) << ": observed=" << bool_label(step.observed);
        if (step.trace_id != 0) {
            ss << " trace_id=" << step.trace_id;
        }
        if (step.active) {
            ss << " active=yes";
        }
        if (step.failed) {
            ss << " status=non-ok";
        }
        ss << "\n";
    }
    return ss.str();
}
