/**
 * @file src/runtime/state/fanout.hpp
 * @brief Runtime state/event fanout helpers.
 */

#ifndef YUNLINK_RUNTIME_STATE_FANOUT_HPP
#define YUNLINK_RUNTIME_STATE_FANOUT_HPP

#include "../core/internal.hpp"

namespace yunlink {

template <typename Payload, typename HandlerMap>
bool runtime_fanout_snapshot(std::mutex& mu,
                             const SecureEnvelope& envelope,
                             const ByteBuffer& bytes,
                             const HandlerMap& source_handlers) {
    Payload payload{};
    if (!decode_typed_payload(bytes, &payload)) {
        return false;
    }
    TypedMessage<Payload> message{envelope, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(message);
        }
    }
    return true;
}

void runtime_publish_semantic_decode_error(EventBus& bus, const EnvelopeEvent& ev);

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_STATE_FANOUT_HPP
