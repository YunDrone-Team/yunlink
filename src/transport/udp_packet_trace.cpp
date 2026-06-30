#include "udp_packet_trace.hpp"

namespace yunlink {

void publish_udp_packet_trace(EventBus* bus,
                              const RuntimeConfig& config,
                              PacketTraceStage stage,
                              const PeerInfo& peer,
                              const SecureEnvelope* envelope,
                              const uint8_t* raw_data,
                              size_t raw_len,
                              ErrorCode code,
                              const std::string& error_message) {
    if (bus == nullptr || !config.packet_trace_enabled) {
        return;
    }
    const TransportType transport =
        envelope != nullptr && envelope->target.scope == TargetScope::kBroadcast
            ? TransportType::kUdpBroadcast
            : TransportType::kUdpUnicast;
    bus->publish_packet_trace(make_packet_trace_record(PacketTraceDirection::kRx,
                                                       stage,
                                                       transport,
                                                       peer,
                                                       envelope,
                                                       raw_data,
                                                       raw_len,
                                                       code,
                                                       error_message,
                                                       config.packet_trace_raw_preview_bytes,
                                                       config.packet_trace_payload_preview_bytes));
}

void publish_udp_parse_events(EventBus* bus,
                              const RuntimeConfig& config,
                              const PeerInfo& peer,
                              const std::vector<EnvelopeStreamParseEvent>& parse_events) {
    for (const auto& parse_event : parse_events) {
        if (parse_event.has_envelope && parse_event.result.ok()) {
            publish_udp_packet_trace(bus,
                                     config,
                                     PacketTraceStage::kDecodeSucceeded,
                                     peer,
                                     &parse_event.result.envelope,
                                     parse_event.raw.data(),
                                     parse_event.raw_len);
        } else {
            publish_udp_packet_trace(bus,
                                     config,
                                     PacketTraceStage::kDecodeFailed,
                                     peer,
                                     nullptr,
                                     parse_event.raw.data(),
                                     parse_event.raw_len,
                                     parse_event.result.code,
                                     parse_event.error_message.empty() ? "udp-decode-failed"
                                                                       : parse_event.error_message);
        }
    }
}

void publish_udp_envelopes(EventBus* bus,
                           const PeerInfo& peer,
                           const std::vector<SecureEnvelope>& envelopes) {
    for (const auto& envelope : envelopes) {
        EnvelopeEvent ev;
        ev.transport = envelope.target.scope == TargetScope::kBroadcast
                           ? TransportType::kUdpBroadcast
                           : TransportType::kUdpUnicast;
        ev.peer = peer;
        ev.envelope = envelope;
        if (bus != nullptr) {
            bus->publish_envelope(ev);
        }
    }
}

}  // namespace yunlink
