/**
 * @file src/runtime/state/fanout.cpp
 * @brief Runtime state/event fanout support.
 */

#include "fanout.hpp"

namespace yunlink {

void runtime_publish_semantic_decode_error(EventBus& bus, const EnvelopeEvent& ev) {
    ErrorEvent error;
    error.code = ErrorCode::kDecodeError;
    error.transport = ev.transport;
    error.peer = ev.peer;
    error.message = "semantic-payload-decode-failed";
    bus.publish_error(error);
}

}  // namespace yunlink
