#ifndef YUNLINK_RUNTIME_CORE_SEND_TRACE_HPP
#define YUNLINK_RUNTIME_CORE_SEND_TRACE_HPP

#include <string>

#include "yunlink/core/event_bus.hpp"
#include "yunlink/core/runtime_config.hpp"

namespace yunlink {

void trace_tx(EventBus& bus,
              const RuntimeConfig& config,
              PacketTraceStage stage,
              TransportType transport,
              const PeerInfo& peer,
              const SecureEnvelope& envelope,
              ErrorCode code = ErrorCode::kOk,
              const std::string& detail = std::string());

void trace_encoded_send(EventBus& bus,
                        const RuntimeConfig& config,
                        TransportType transport,
                        const PeerInfo& peer,
                        const SecureEnvelope& envelope);

void trace_send_result(EventBus& bus,
                       const RuntimeConfig& config,
                       TransportType transport,
                       const PeerInfo& peer,
                       const SecureEnvelope& envelope,
                       ErrorCode result,
                       const std::string& failed_detail);

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_CORE_SEND_TRACE_HPP
