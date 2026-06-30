#include "packets/flow/packet_flow_model.hpp"

#include <algorithm>
#include <sstream>
#include <unordered_map>

#include "packets/flow/packet_flow_format.hpp"
#include "packets/format/packet_trace_format.hpp"

namespace {

constexpr uint64_t kSelectedFallbackWindowMs = 3000;

std::vector<PacketFlowStage> ordered_stages() {
    return {PacketFlowStage::kSemanticPayload,
            PacketFlowStage::kSecureEnvelope,
            PacketFlowStage::kEncode,
            PacketFlowStage::kTransportSend,
            PacketFlowStage::kTransportReceive,
            PacketFlowStage::kDecode,
            PacketFlowStage::kRuntimeDispatch,
            PacketFlowStage::kSemanticSubscriber};
}

bool record_failed(const yunlink::PacketTraceRecord& record) {
    return packet_trace_is_error(record);
}

std::string record_semantic_label(const yunlink::PacketTraceRecord& record) {
    if (!record.has_envelope) {
        return "YunLink packet";
    }
    return packet_family_label(record.envelope.message_family) + " / " +
           packet_message_type_label(record.envelope.message_family, record.envelope.message_type);
}

std::string record_subtitle(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << packet_direction_label(record.direction) << " "
       << packet_transport_trace_label(record.transport);
    if (!record.peer.id.empty()) {
        ss << " " << record.peer.id;
    }
    return ss.str();
}

std::string record_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << "trace_id: " << record.trace_id << "\n";
    ss << "stage: " << packet_stage_label(record.stage) << "\n";
    ss << "direction: " << packet_direction_label(record.direction) << "\n";
    ss << "transport: " << packet_transport_trace_label(record.transport) << "\n";
    if (!record.peer.id.empty()) {
        ss << "peer: " << record.peer.id << "\n";
    }
    if (record.has_envelope) {
        ss << "family: " << packet_family_label(record.envelope.message_family) << "\n";
        ss << "type: "
           << packet_message_type_label(record.envelope.message_family,
                                        record.envelope.message_type)
           << "\n";
        ss << "qos: " << packet_qos_label(record.envelope.qos_class) << "\n";
        ss << "session: " << record.envelope.session_id << "\n";
        ss << "message: " << record.envelope.message_id << "\n";
        ss << "correlation: " << record.envelope.correlation_id << "\n";
    }
    if (record.code != yunlink::ErrorCode::kOk) {
        ss << "status: " << packet_status_label(record) << "\n";
    }
    return ss.str();
}

std::vector<PacketFlowStage> stages_for_record(const yunlink::PacketTraceRecord& record) {
    switch (record.stage) {
    case yunlink::PacketTraceStage::kEncodedForSend:
        return {PacketFlowStage::kSemanticPayload,
                PacketFlowStage::kSecureEnvelope,
                PacketFlowStage::kEncode,
                PacketFlowStage::kTransportSend};
    case yunlink::PacketTraceStage::kSendSucceeded:
    case yunlink::PacketTraceStage::kSendFailed:
        return {PacketFlowStage::kTransportSend};
    case yunlink::PacketTraceStage::kRawReceived:
        return {PacketFlowStage::kTransportReceive};
    case yunlink::PacketTraceStage::kDecodeSucceeded:
        return {PacketFlowStage::kSecureEnvelope, PacketFlowStage::kDecode};
    case yunlink::PacketTraceStage::kDecodeFailed:
        return {PacketFlowStage::kDecode};
    case yunlink::PacketTraceStage::kDispatchAccepted:
        return {PacketFlowStage::kRuntimeDispatch, PacketFlowStage::kSemanticSubscriber};
    case yunlink::PacketTraceStage::kDispatchRejected:
        return {PacketFlowStage::kRuntimeDispatch};
    }
    return {};
}

std::string journey_key(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    if (record.has_envelope &&
        (record.envelope.message_id != 0 || record.envelope.correlation_id != 0 ||
         record.envelope.session_id != 0)) {
        ss << "env:" << static_cast<int>(record.direction) << ":" << record.peer.id << ":"
           << record.envelope.session_id << ":" << record.envelope.message_id << ":"
           << record.envelope.correlation_id;
        return ss.str();
    }
    ss << "trace:" << record.trace_id;
    return ss.str();
}

bool same_selected_journey(const yunlink::PacketTraceRecord& record,
                           const yunlink::PacketTraceRecord& selected) {
    if (record.has_envelope && selected.has_envelope &&
        (selected.envelope.message_id != 0 || selected.envelope.correlation_id != 0 ||
         selected.envelope.session_id != 0)) {
        return record.direction == selected.direction && record.peer.id == selected.peer.id &&
               record.envelope.session_id == selected.envelope.session_id &&
               record.envelope.message_id == selected.envelope.message_id &&
               record.envelope.correlation_id == selected.envelope.correlation_id;
    }
    const uint64_t a = record.observed_at_ms;
    const uint64_t b = selected.observed_at_ms;
    const uint64_t delta = a > b ? a - b : b - a;
    return record.direction == selected.direction && delta <= kSelectedFallbackWindowMs;
}

PacketFlowStep make_placeholder(PacketFlowStage stage) {
    PacketFlowStep step;
    step.stage = stage;
    step.title = packet_flow_stage_title(stage);
    step.subtitle = "not observed";
    step.detail = "This stage is part of the YunLink lifecycle but is not present in the current trace window.";
    return step;
}

void merge_record(PacketFlowJourney* journey, const yunlink::PacketTraceRecord& record) {
    if (journey == nullptr) {
        return;
    }
    journey->newest_trace_id = std::max(journey->newest_trace_id, record.trace_id);
    journey->newest_at_ms = std::max(journey->newest_at_ms, record.observed_at_ms);
    const auto mapped = stages_for_record(record);
    for (auto stage : mapped) {
        auto it = std::find_if(journey->steps.begin(),
                               journey->steps.end(),
                               [stage](const PacketFlowStep& step) { return step.stage == stage; });
        if (it == journey->steps.end()) {
            continue;
        }
        if (!it->observed || record.observed_at_ms >= it->observed_at_ms) {
            it->observed = true;
            it->observed_at_ms = record.observed_at_ms;
            it->trace_id = record.trace_id;
            it->title = packet_flow_stage_title(stage);
            it->subtitle = record_subtitle(record);
            it->detail = record_detail(record);
            it->failed = record_failed(record);
        }
    }
}

PacketFlowJourney journey_from_records(const std::vector<yunlink::PacketTraceRecord>& records) {
    if (records.empty()) {
        return packet_flow_empty_journey("No packet journey", "Waiting for YunLink trace records");
    }
    auto newest = std::max_element(records.begin(), records.end(), [](const auto& a, const auto& b) {
        return a.trace_id < b.trace_id;
    });
    PacketFlowJourney journey =
        packet_flow_empty_journey(record_semantic_label(*newest), record_subtitle(*newest));
    for (const auto& record : records) {
        merge_record(&journey, record);
    }
    for (auto& step : journey.steps) {
        step.active = step.trace_id != 0 && step.trace_id == journey.newest_trace_id;
    }
    return journey;
}

}  // namespace

PacketFlowJourney packet_flow_empty_journey(const std::string& title,
                                            const std::string& subtitle) {
    PacketFlowJourney journey;
    journey.title = title;
    journey.subtitle = subtitle;
    for (auto stage : ordered_stages()) {
        journey.steps.push_back(make_placeholder(stage));
    }
    return journey;
}

PacketFlowSnapshot packet_flow_live_snapshot(
    const std::vector<yunlink::PacketTraceRecord>& records,
    uint64_t now_ms,
    uint64_t window_ms,
    size_t max_journeys) {
    PacketFlowSnapshot snapshot;
    snapshot.mode = PacketFlowMode::kLive;
    snapshot.generated_at_ms = now_ms;

    std::unordered_map<std::string, std::vector<yunlink::PacketTraceRecord>> grouped;
    for (const auto& record : records) {
        if (window_ms != 0 && now_ms > record.observed_at_ms &&
            now_ms - record.observed_at_ms > window_ms) {
            continue;
        }
        snapshot.newest_trace_id = std::max(snapshot.newest_trace_id, record.trace_id);
        grouped[journey_key(record)].push_back(record);
    }

    for (const auto& entry : grouped) {
        snapshot.journeys.push_back(journey_from_records(entry.second));
    }
    std::sort(snapshot.journeys.begin(), snapshot.journeys.end(), [](const auto& a, const auto& b) {
        return a.newest_at_ms > b.newest_at_ms;
    });
    if (snapshot.journeys.size() > max_journeys) {
        snapshot.journeys.resize(max_journeys);
    }
    if (snapshot.journeys.empty()) {
        snapshot.journeys.push_back(
            packet_flow_empty_journey("Live Flow", "Waiting for YunLink packet traces"));
    }
    return snapshot;
}

PacketFlowSnapshot packet_flow_selected_snapshot(
    const std::vector<yunlink::PacketTraceRecord>& records,
    uint64_t selected_trace_id,
    uint64_t now_ms) {
    PacketFlowSnapshot snapshot;
    snapshot.mode = PacketFlowMode::kSelected;
    snapshot.generated_at_ms = now_ms;

    const auto selected = std::find_if(records.begin(), records.end(), [selected_trace_id](const auto& record) {
        return record.trace_id == selected_trace_id;
    });
    if (selected == records.end()) {
        snapshot.journeys.push_back(
            packet_flow_empty_journey("Selected Flow", "Select a packet in the Table tab"));
        return snapshot;
    }

    std::vector<yunlink::PacketTraceRecord> journey_records;
    for (const auto& record : records) {
        if (same_selected_journey(record, *selected)) {
            journey_records.push_back(record);
            snapshot.newest_trace_id = std::max(snapshot.newest_trace_id, record.trace_id);
        }
    }
    snapshot.journeys.push_back(journey_from_records(journey_records));
    return snapshot;
}
