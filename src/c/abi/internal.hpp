/**
 * @file src/c/abi/internal.hpp
 * @brief Shared C ABI implementation helpers.
 */

#ifndef YUNLINK_C_ABI_INTERNAL_HPP
#define YUNLINK_C_ABI_INTERNAL_HPP

#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include <vector>

#include "yunlink/c/yunlink_c.h"
#include "yunlink/runtime/runtime.hpp"

struct yunlink_runtime {
    yunlink::Runtime runtime;
    std::mutex mu;
    std::deque<yunlink_runtime_event_t> queue;
    size_t tok_error = 0;
    size_t tok_link = 0;
    size_t tok_vehicle_core = 0;
    size_t tok_px4_state = 0;
    size_t tok_local_odom = 0;
    size_t tok_host_system = 0;
    size_t tok_authority_status = 0;
    size_t tok_vehicle_event = 0;
    size_t tok_command_result = 0;
    std::vector<size_t> configuration_tokens;
    std::vector<size_t> system_service_tokens;
    size_t tok_feature_list = 0;
    size_t tok_feature_get = 0;
    size_t tok_feature_start = 0;
    size_t tok_topic_list = 0;
    size_t tok_topic_subscription = 0;
    size_t tok_topic_sample = 0;
    bool started = false;
};

namespace yunlink_c_abi {

void safe_copy(char* dst, size_t cap, const std::string& src);
yunlink_result_t to_result(yunlink::ErrorCode code);
yunlink::TargetSelector to_target_selector(const yunlink_target_selector_t& target);
yunlink_target_selector_t to_c_target_selector(const yunlink::TargetSelector& target);
void to_c_peer(const std::string& peer_id, yunlink_peer_t* out_peer);
yunlink::RuntimeConfig to_runtime_config(const yunlink_runtime_config_t& cfg);
yunlink_identity_t to_c_identity(const yunlink::EndpointIdentity& identity);
yunlink::EndpointIdentity to_identity(const yunlink_identity_t& identity);

void push_event(yunlink_runtime_t* runtime, const yunlink_runtime_event_t& event);
void clear_queue(yunlink_runtime_t* runtime);
void subscribe_runtime_events(yunlink_runtime_t* runtime);
void unsubscribe_runtime_events(yunlink_runtime_t* runtime);
void unsubscribe_configuration_callbacks(yunlink_runtime_t* runtime);
void unsubscribe_system_service_callbacks(yunlink_runtime_t* runtime);

bool validate_input_runtime(yunlink_runtime_t* runtime);
bool validate_peer(const yunlink_peer_t* peer);
bool validate_session(const yunlink_session_t* session);
bool validate_target(const yunlink_target_selector_t* target);
void fill_command_handle(const yunlink::CommandHandle& in, yunlink_command_handle_t* out);

}  // namespace yunlink_c_abi

#endif  // YUNLINK_C_ABI_INTERNAL_HPP
