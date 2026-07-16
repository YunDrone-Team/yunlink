/**
 * @file include/yunlink/c/abi/functions.h
 * @brief C ABI function declarations.
 */

#ifndef YUNLINK_C_ABI_FUNCTIONS_H
#define YUNLINK_C_ABI_FUNCTIONS_H

#include "yunlink/c/abi/types.h"

#ifdef __cplusplus
extern "C" {
#endif

YUNLINK_C_API uint32_t yunlink_ffi_abi_version(void);
YUNLINK_C_API const char* yunlink_result_name(yunlink_result_t result);

YUNLINK_C_API yunlink_result_t yunlink_runtime_create(yunlink_runtime_t** out_runtime);
YUNLINK_C_API void yunlink_runtime_destroy(yunlink_runtime_t* runtime);

YUNLINK_C_API yunlink_result_t yunlink_runtime_start(yunlink_runtime_t* runtime,
                                                     const yunlink_runtime_config_t* cfg);
YUNLINK_C_API yunlink_result_t yunlink_runtime_stop(yunlink_runtime_t* runtime);

YUNLINK_C_API yunlink_result_t yunlink_peer_connect(yunlink_runtime_t* runtime,
                                                    const char* ip,
                                                    uint16_t port,
                                                    yunlink_peer_t* out_peer);
YUNLINK_C_API yunlink_result_t yunlink_session_open(yunlink_runtime_t* runtime,
                                                    const yunlink_peer_t* peer,
                                                    const char* node_name,
                                                    yunlink_session_t* out_session);
YUNLINK_C_API yunlink_result_t yunlink_session_describe(yunlink_runtime_t* runtime,
                                                        const yunlink_session_t* session,
                                                        yunlink_session_info_t* out_info);

YUNLINK_C_API yunlink_result_t yunlink_authority_request(yunlink_runtime_t* runtime,
                                                         const yunlink_peer_t* peer,
                                                         const yunlink_session_t* session,
                                                         const yunlink_target_selector_t* target,
                                                         uint8_t source,
                                                         uint32_t lease_ttl_ms,
                                                         uint8_t allow_preempt);
YUNLINK_C_API yunlink_result_t yunlink_authority_renew(yunlink_runtime_t* runtime,
                                                       const yunlink_peer_t* peer,
                                                       const yunlink_session_t* session,
                                                       const yunlink_target_selector_t* target,
                                                       uint8_t source,
                                                       uint32_t lease_ttl_ms);
YUNLINK_C_API yunlink_result_t yunlink_authority_release(yunlink_runtime_t* runtime,
                                                         const yunlink_peer_t* peer,
                                                         const yunlink_session_t* session,
                                                         const yunlink_target_selector_t* target);
YUNLINK_C_API yunlink_result_t yunlink_authority_current(yunlink_runtime_t* runtime,
                                                         yunlink_authority_lease_t* out_lease);

YUNLINK_C_API yunlink_result_t
yunlink_command_publish_takeoff(yunlink_runtime_t* runtime,
                                const yunlink_peer_t* peer,
                                const yunlink_session_t* session,
                                const yunlink_target_selector_t* target,
                                const yunlink_takeoff_command_t* payload,
                                yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t yunlink_command_publish_land(yunlink_runtime_t* runtime,
                                                            const yunlink_peer_t* peer,
                                                            const yunlink_session_t* session,
                                                            const yunlink_target_selector_t* target,
                                                            const yunlink_land_command_t* payload,
                                                            yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_command_publish_return(yunlink_runtime_t* runtime,
                               const yunlink_peer_t* peer,
                               const yunlink_session_t* session,
                               const yunlink_target_selector_t* target,
                               const yunlink_return_command_t* payload,
                               yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t yunlink_command_publish_goto(yunlink_runtime_t* runtime,
                                                            const yunlink_peer_t* peer,
                                                            const yunlink_session_t* session,
                                                            const yunlink_target_selector_t* target,
                                                            const yunlink_goto_command_t* payload,
                                                            yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_command_publish_velocity_setpoint(yunlink_runtime_t* runtime,
                                          const yunlink_peer_t* peer,
                                          const yunlink_session_t* session,
                                          const yunlink_target_selector_t* target,
                                          const yunlink_velocity_setpoint_command_t* payload,
                                          yunlink_command_handle_t* out_handle);

YUNLINK_C_API yunlink_result_t
yunlink_publish_vehicle_core_state(yunlink_runtime_t* runtime,
                                   const yunlink_peer_t* peer,
                                   const yunlink_target_selector_t* target,
                                   const yunlink_vehicle_core_state_t* payload,
                                   uint64_t session_id);

YUNLINK_C_API yunlink_result_t
yunlink_system_service_request_feature_list(yunlink_runtime_t* runtime,
                                            const yunlink_peer_t* peer,
                                            const yunlink_session_t* session,
                                            const yunlink_target_selector_t* target,
                                            yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_system_service_request_feature_get(yunlink_runtime_t* runtime,
                                           const yunlink_peer_t* peer,
                                           const yunlink_session_t* session,
                                           const yunlink_target_selector_t* target,
                                           const char* feature_name,
                                           yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_system_service_request_runtime_log_list(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                yunlink_command_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_system_service_request_runtime_log_read(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                yunlink_string_view_t runtime_id,
                                                uint64_t cursor,
                                                uint32_t max_bytes,
                                                yunlink_command_handle_t* out_handle);

YUNLINK_C_API yunlink_result_t yunlink_system_service_subscribe_runtime_log_list_responses(
    yunlink_runtime_t* runtime,
    yunlink_runtime_log_list_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_system_service_subscribe_runtime_log_read_responses(
    yunlink_runtime_t* runtime,
    yunlink_runtime_log_read_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_system_service_unsubscribe(yunlink_runtime_t* runtime,
                                                                   size_t token);

YUNLINK_C_API yunlink_result_t yunlink_runtime_poll_event(yunlink_runtime_t* runtime,
                                                          yunlink_runtime_event_t* out_event);
YUNLINK_C_API yunlink_result_t
yunlink_runtime_poll_command_result(yunlink_runtime_t* runtime,
                                    yunlink_command_result_event_t* out_event);
YUNLINK_C_API yunlink_result_t
yunlink_runtime_poll_vehicle_core_state(yunlink_runtime_t* runtime,
                                        yunlink_vehicle_core_state_event_t* out_event);

YUNLINK_C_API yunlink_result_t
yunlink_configuration_publish_resource_list_request(yunlink_runtime_t* runtime,
                                                    const yunlink_peer_t* peer,
                                                    const yunlink_session_t* session,
                                                    const yunlink_target_selector_t* target,
                                                    yunlink_configuration_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_configuration_publish_resource_describe_request(yunlink_runtime_t* runtime,
                                                        const yunlink_peer_t* peer,
                                                        const yunlink_session_t* session,
                                                        const yunlink_target_selector_t* target,
                                                        yunlink_string_view_t resource_id,
                                                        yunlink_configuration_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_configuration_publish_resource_get_request(yunlink_runtime_t* runtime,
                                                   const yunlink_peer_t* peer,
                                                   const yunlink_session_t* session,
                                                   const yunlink_target_selector_t* target,
                                                   yunlink_string_view_t resource_id,
                                                   yunlink_configuration_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t yunlink_configuration_publish_resource_patch_request(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    yunlink_string_view_t resource_id,
    yunlink_string_view_t expected_revision,
    const yunlink_config_field_value_view_t* updates,
    size_t update_count,
    uint8_t validate_only,
    yunlink_configuration_handle_t* out_handle);
YUNLINK_C_API yunlink_result_t
yunlink_configuration_publish_resource_apply_request(yunlink_runtime_t* runtime,
                                                     const yunlink_peer_t* peer,
                                                     const yunlink_session_t* session,
                                                     const yunlink_target_selector_t* target,
                                                     yunlink_string_view_t resource_id,
                                                     yunlink_string_view_t expected_revision,
                                                     yunlink_configuration_handle_t* out_handle);

YUNLINK_C_API yunlink_result_t yunlink_configuration_subscribe_resource_list_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_list_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_configuration_subscribe_resource_describe_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_describe_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_configuration_subscribe_resource_get_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_get_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_configuration_subscribe_resource_patch_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_patch_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_configuration_subscribe_resource_apply_responses(
    yunlink_runtime_t* runtime,
    yunlink_config_resource_apply_response_callback_t callback,
    void* user_data,
    size_t* out_token);
YUNLINK_C_API yunlink_result_t yunlink_configuration_unsubscribe(yunlink_runtime_t* runtime,
                                                                 size_t token);

#ifdef __cplusplus
}
#endif

#endif  // YUNLINK_C_ABI_FUNCTIONS_H
