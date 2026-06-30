/**
 * @file include/yunlink/runtime/events.hpp
 * @brief Runtime semantic event subscription facade declarations.
 */

#ifndef YUNLINK_RUNTIME_EVENTS_HPP
#define YUNLINK_RUNTIME_EVENTS_HPP

#include <cstddef>
#include <functional>

#include "yunlink/diagnostics/packet_trace.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace yunlink {

class Runtime;

class EventSubscriber {
  public:
    using VehicleEventHandler = std::function<void(const TypedMessage<VehicleEvent>&)>;
    using CommandResultHandler = std::function<void(const CommandResultView&)>;
    using AuthorityStatusHandler = std::function<void(const TypedMessage<AuthorityStatus>&)>;
    using BulkChannelDescriptorHandler =
        std::function<void(const TypedMessage<BulkChannelDescriptor>&)>;
    using PacketTraceHandler = std::function<void(const PacketTraceRecord&)>;

    explicit EventSubscriber(Runtime* runtime = nullptr);

    size_t subscribe_vehicle_event(VehicleEventHandler cb);
    size_t subscribe_command_results(CommandResultHandler cb);
    size_t subscribe_authority_status(AuthorityStatusHandler cb);
    size_t subscribe_bulk_channel_descriptors(BulkChannelDescriptorHandler cb);
    size_t subscribe_packet_trace(PacketTraceHandler cb);
    void unsubscribe(size_t token);
    void bind(Runtime* runtime);

  private:
    Runtime* runtime_ = nullptr;
};

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_EVENTS_HPP
