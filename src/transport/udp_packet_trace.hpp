#ifndef YUNLINK_TRANSPORT_UDP_PACKET_TRACE_HPP
#define YUNLINK_TRANSPORT_UDP_PACKET_TRACE_HPP

#include <vector>

#include "yunlink/core/envelope_stream_parser.hpp"
#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/runtime_config.hpp"

namespace yunlink {

void publish_udp_packet_trace(EventBus* bus,
                              const RuntimeConfig& config,
                              PacketTraceStage stage,
                              const PeerInfo& peer,
                              const SecureEnvelope* envelope,
                              const uint8_t* raw_data,
                              size_t raw_len,
                              ErrorCode code = ErrorCode::kOk,
                              const std::string& error_message = std::string());

void publish_udp_parse_events(EventBus* bus,
                              const RuntimeConfig& config,
                              const PeerInfo& peer,
                              const std::vector<EnvelopeStreamParseEvent>& parse_events);

void publish_udp_envelopes(EventBus* bus,
                           const PeerInfo& peer,
                           const std::vector<SecureEnvelope>& envelopes);

}  // namespace yunlink

#endif  // YUNLINK_TRANSPORT_UDP_PACKET_TRACE_HPP
