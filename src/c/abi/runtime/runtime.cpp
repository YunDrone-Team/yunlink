/**
 * @file src/c/abi/runtime.cpp
 * @brief C ABI runtime lifecycle functions.
 */

#include "../internal.hpp"

using namespace yunlink_c_abi;

extern "C" {

uint32_t yunlink_ffi_abi_version(void) {
    return YUNLINK_FFI_ABI_VERSION;
}

yunlink_result_t yunlink_runtime_create(yunlink_runtime_t** out_runtime) {
    if (out_runtime == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    *out_runtime = new yunlink_runtime();
    return YUNLINK_RESULT_OK;
}

void yunlink_runtime_destroy(yunlink_runtime_t* runtime) {
    if (!runtime) {
        return;
    }
    yunlink_runtime_stop(runtime);
    delete runtime;
}

yunlink_result_t yunlink_runtime_start(yunlink_runtime_t* runtime,
                                       const yunlink_runtime_config_t* cfg) {
    constexpr size_t kLegacyConfigSize =
        offsetof(yunlink_runtime_config_t, qos_udp_fallback_to_tcp) + sizeof(uint8_t);
    if (!validate_input_runtime(runtime) || cfg == nullptr ||
        cfg->struct_size < kLegacyConfigSize) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (runtime->started) {
        return YUNLINK_RESULT_OK;
    }
    clear_queue(runtime);
    const auto result = to_result(runtime->runtime.start(to_runtime_config(*cfg)));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    subscribe_runtime_events(runtime);
    runtime->started = true;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_runtime_stop(yunlink_runtime_t* runtime) {
    if (!validate_input_runtime(runtime)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (!runtime->started) {
        unsubscribe_configuration_callbacks(runtime);
        clear_queue(runtime);
        return YUNLINK_RESULT_OK;
    }
    unsubscribe_configuration_callbacks(runtime);
    unsubscribe_runtime_events(runtime);
    runtime->runtime.stop();
    runtime->started = false;
    clear_queue(runtime);
    return YUNLINK_RESULT_OK;
}

}  // extern "C"
