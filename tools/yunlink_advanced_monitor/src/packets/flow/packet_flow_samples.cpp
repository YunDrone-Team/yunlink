#include "packets/flow/packet_flow_samples.hpp"

#include <algorithm>

#include "packets/flow/packet_flow_format.hpp"

namespace {

PacketFlowStep demo_step(PacketFlowStage stage,
                         uint64_t observed_at_ms,
                         const std::string& subtitle,
                         const std::string& detail) {
    PacketFlowStep step;
    step.stage = stage;
    step.observed_at_ms = observed_at_ms;
    step.title = packet_flow_stage_title(stage);
    step.subtitle = subtitle;
    step.detail = detail;
    step.observed = true;
    return step;
}

}  // namespace

PacketFlowSnapshot packet_flow_takeoff_demo_snapshot(size_t active_step, uint64_t now_ms) {
    PacketFlowSnapshot snapshot;
    snapshot.mode = PacketFlowMode::kDemo;
    snapshot.generated_at_ms = now_ms;

    PacketFlowJourney journey;
    journey.title = "TAKEOFF Command";
    journey.subtitle = "Command / TAKEOFF over reliable ordered TCP";
    journey.newest_trace_id = static_cast<uint64_t>(active_step + 1);
    journey.newest_at_ms = now_ms;
    journey.steps = {
        demo_step(PacketFlowStage::kSemanticPayload,
                  now_ms,
                  "TakeoffCommand",
                  "reserved=0\nAction-only command; vehicle uses its current control configuration."),
        demo_step(PacketFlowStage::kSecureEnvelope,
                  now_ms,
                  "family=Command type=TAKEOFF",
                  "qos=ReliableOrdered\nsession_id=42\nmessage_id=1001\ncorrelation_id=1001\ntarget=uav1"),
        demo_step(PacketFlowStage::kEncode,
                  now_ms,
                  "ProtocolCodec encode",
                  "magic=SURY\nfixed header + variable header + payload + checksum\nNo runtime trace store write."),
        demo_step(PacketFlowStage::kTransportSend,
                  now_ms,
                  "TCP session send",
                  "The demo stops at an in-memory step.\nIt does not open sockets or send a command."),
        demo_step(PacketFlowStage::kTransportReceive,
                  now_ms,
                  "peer TCP stream receive",
                  "A receiving runtime would read the YunLink frame from its transport buffer."),
        demo_step(PacketFlowStage::kDecode,
                  now_ms,
                  "ProtocolCodec decode",
                  "Header, payload length, checksum, target ids and auth tag become a SecureEnvelope."),
        demo_step(PacketFlowStage::kRuntimeDispatch,
                  now_ms,
                  "runtime checks and dispatch",
                  "target match, schema, security and ttl checks happen before subscriber delivery."),
        demo_step(PacketFlowStage::kSemanticSubscriber,
                  now_ms,
                  "Command subscriber",
                  "A semantic handler receives TakeoffCommand.\nBusiness completion semantics are not inferred here."),
    };
    active_step = std::min(active_step, journey.steps.empty() ? size_t{0} : journey.steps.size() - 1);
    for (size_t i = 0; i < journey.steps.size(); ++i) {
        journey.steps[i].trace_id = static_cast<uint64_t>(i + 1);
        journey.steps[i].active = i == active_step;
    }
    snapshot.newest_trace_id = journey.steps[active_step].trace_id;
    snapshot.journeys.push_back(std::move(journey));
    return snapshot;
}
