#include "send_trace.hpp"

#include "internal.hpp"

namespace yunlink {

void trace_tx(EventBus& bus,
              const RuntimeConfig& config,
              PacketTraceStage stage,
              TransportType transport,
              const PeerInfo& peer,
              const SecureEnvelope& envelope,
              ErrorCode code,
              const std::string& detail) {
    runtime_publish_packet_trace(bus,
                                 config,
                                 PacketTraceDirection::kTx,
                                 stage,
                                 transport,
                                 peer,
                                 envelope,
                                 nullptr,
                                 0,
                                 code,
                                 detail);
}

void trace_encoded_send(EventBus& bus,
                        const RuntimeConfig& config,
                        TransportType transport,
                        const PeerInfo& peer,
                        const SecureEnvelope& envelope) {
    trace_tx(bus, config, PacketTraceStage::kEncodedForSend, transport, peer, envelope);
}

void trace_send_result(EventBus& bus,
                       const RuntimeConfig& config,
                       TransportType transport,
                       const PeerInfo& peer,
                       const SecureEnvelope& envelope,
                       ErrorCode result,
                       const std::string& failed_detail) {
    const PacketTraceStage stage =
        result == ErrorCode::kOk ? PacketTraceStage::kSendSucceeded : PacketTraceStage::kSendFailed;
    trace_tx(bus,
             config,
             stage,
             transport,
             peer,
             envelope,
             result,
             result == ErrorCode::kOk ? std::string() : failed_detail);
}

}  // namespace yunlink
