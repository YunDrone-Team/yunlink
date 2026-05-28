/**
 * @file src/c/abi/events.cpp
 * @brief C ABI runtime event polling functions.
 */

#include "../internal.hpp"

#include <cstring>

using namespace yunlink_c_abi;

extern "C" {

yunlink_result_t yunlink_runtime_poll_event(yunlink_runtime_t* runtime,
                                            yunlink_runtime_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    if (runtime->queue.empty()) {
        std::memset(out_event, 0, sizeof(*out_event));
        out_event->type = YUNLINK_RUNTIME_EVENT_NONE;
        return YUNLINK_RESULT_OK;
    }
    *out_event = runtime->queue.front();
    runtime->queue.pop_front();
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_runtime_poll_command_result(yunlink_runtime_t* runtime,
                                                     yunlink_command_result_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    for (auto it = runtime->queue.begin(); it != runtime->queue.end(); ++it) {
        if (it->type != YUNLINK_RUNTIME_EVENT_COMMAND_RESULT) {
            continue;
        }
        *out_event = it->data.command_result;
        runtime->queue.erase(it);
        return YUNLINK_RESULT_OK;
    }
    std::memset(out_event, 0, sizeof(*out_event));
    return YUNLINK_RESULT_NOT_FOUND;
}

yunlink_result_t
yunlink_runtime_poll_vehicle_core_state(yunlink_runtime_t* runtime,
                                        yunlink_vehicle_core_state_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    for (auto it = runtime->queue.begin(); it != runtime->queue.end(); ++it) {
        if (it->type != YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE) {
            continue;
        }
        *out_event = it->data.vehicle_core_state;
        runtime->queue.erase(it);
        return YUNLINK_RESULT_OK;
    }
    std::memset(out_event, 0, sizeof(*out_event));
    return YUNLINK_RESULT_NOT_FOUND;
}

}  // extern "C"
