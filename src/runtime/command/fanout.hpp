/**
 * @file src/runtime/command/fanout.hpp
 * @brief Command subscriber fanout helpers.
 */

#ifndef YUNLINK_RUNTIME_COMMAND_FANOUT_HPP
#define YUNLINK_RUNTIME_COMMAND_FANOUT_HPP

#include "../core/internal.hpp"

namespace yunlink {

template <typename Payload, typename HandlerMap>
bool runtime_fanout_command(std::mutex& mu,
                            const EnvelopeEvent& ev,
                            const ByteBuffer& bytes,
                            const HandlerMap& source_handlers) {
    Payload payload{};
    if (!decode_typed_payload(bytes, &payload)) {
        return false;
    }

    InboundCommandView<Payload> view{ev, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(view);
        }
    }
    return true;
}

template <typename Payload, typename HandlerMap>
void runtime_fanout_command_payload(std::mutex& mu,
                                    const EnvelopeEvent& ev,
                                    const Payload& payload,
                                    const HandlerMap& source_handlers) {
    InboundCommandView<Payload> view{ev, payload};
    HandlerMap handlers;
    {
        std::lock_guard<std::mutex> lock(mu);
        handlers = source_handlers;
    }
    for (const auto& item : handlers) {
        if (item.second) {
            item.second(view);
        }
    }
}

}  // namespace yunlink

#endif  // YUNLINK_RUNTIME_COMMAND_FANOUT_HPP
