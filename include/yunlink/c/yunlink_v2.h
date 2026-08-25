/**
 * @file include/yunlink/c/yunlink_v2.h
 * @brief Stable generic C ABI for YunLink Wire v2.
 */

#ifndef YUNLINK_C_YUNLINK_V2_H
#define YUNLINK_C_YUNLINK_V2_H

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32) && defined(YUNLINK_FFI_SHARED_BUILD)
#define YUNLINK_V2_API __declspec(dllexport)
#elif defined(_WIN32)
#define YUNLINK_V2_API __declspec(dllimport)
#else
#define YUNLINK_V2_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define YUNLINK_V2_ABI_VERSION 2U

typedef struct yunlink_v2_runtime yunlink_v2_runtime_t;
typedef struct yunlink_v2_discovery_advertisement yunlink_v2_discovery_advertisement_t;

typedef struct yunlink_v2_string_view {
    const char* data;
    size_t len;
} yunlink_v2_string_view_t;

typedef struct yunlink_v2_bytes_view {
    const uint8_t* data;
    size_t len;
} yunlink_v2_bytes_view_t;

typedef struct yunlink_v2_profile_view {
    yunlink_v2_string_view_t profile_id;
    uint16_t major;
    uint16_t minor;
    yunlink_v2_string_view_t schema_digest;
} yunlink_v2_profile_view_t;

typedef struct yunlink_v2_type_ref_view {
    yunlink_v2_string_view_t profile_id;
    uint16_t major;
    uint16_t minor;
    yunlink_v2_string_view_t type_name;
} yunlink_v2_type_ref_view_t;

typedef struct yunlink_v2_runtime_config {
    size_t struct_size;
    yunlink_v2_string_view_t endpoint_uid;
    yunlink_v2_string_view_t display_name;
    yunlink_v2_string_view_t shared_secret;
    uint16_t tcp_listen_port;
    const yunlink_v2_profile_view_t* profiles;
    size_t profile_count;
    const yunlink_v2_profile_view_t* required_profiles;
    size_t required_profile_count;
} yunlink_v2_runtime_config_t;

typedef struct yunlink_v2_peer {
    char id[256];
    char ip[64];
    uint16_t port;
} yunlink_v2_peer_t;

typedef struct yunlink_v2_key_value_view {
    yunlink_v2_string_view_t key;
    yunlink_v2_string_view_t value;
} yunlink_v2_key_value_view_t;

typedef struct yunlink_v2_discovery_entity_view {
    yunlink_v2_string_view_t entity_uid;
    yunlink_v2_string_view_t kind;
    yunlink_v2_string_view_t display_name;
    uint8_t availability;
    uint32_t agent_id;
} yunlink_v2_discovery_entity_view_t;

typedef struct yunlink_v2_target_view {
    uint8_t scope;
    const yunlink_v2_string_view_t* uids;
    size_t uid_count;
} yunlink_v2_target_view_t;

typedef struct yunlink_v2_message_handle {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
} yunlink_v2_message_handle_t;

typedef struct yunlink_v2_event {
    uint8_t kind;
    yunlink_v2_string_view_t peer_id;
    uint8_t link_up;
    uint16_t error_code;
    yunlink_v2_string_view_t message;
    uint8_t session_state;
    uint8_t session_authenticated;
    uint64_t session_id;
    uint8_t family;
    uint8_t operation;
    uint8_t qos_class;
    uint64_t message_id;
    uint64_t correlation_id;
    uint64_t created_at_ms;
    uint32_t ttl_ms;
    yunlink_v2_string_view_t source_endpoint_uid;
    yunlink_v2_string_view_t source_entity_uid;
    yunlink_v2_target_view_t target;
    yunlink_v2_type_ref_view_t type_ref;
    yunlink_v2_bytes_view_t payload;
} yunlink_v2_event_t;

typedef void (*yunlink_v2_event_callback_t)(const yunlink_v2_event_t* event, void* user_data);

YUNLINK_V2_API uint32_t yunlink_v2_abi_version(void);
YUNLINK_V2_API yunlink_v2_runtime_t* yunlink_v2_runtime_create(void);
YUNLINK_V2_API void yunlink_v2_runtime_destroy(yunlink_v2_runtime_t* runtime);
YUNLINK_V2_API uint16_t yunlink_v2_runtime_start(yunlink_v2_runtime_t* runtime,
                                                 const yunlink_v2_runtime_config_t* config);
YUNLINK_V2_API void yunlink_v2_runtime_stop(yunlink_v2_runtime_t* runtime);
YUNLINK_V2_API uint16_t yunlink_v2_runtime_listening_port(
    const yunlink_v2_runtime_t* runtime);
YUNLINK_V2_API uint16_t yunlink_v2_runtime_connect(yunlink_v2_runtime_t* runtime,
                                                   yunlink_v2_string_view_t ip,
                                                   uint16_t port,
                                                   yunlink_v2_peer_t* out_peer);
YUNLINK_V2_API void yunlink_v2_runtime_close_peer(yunlink_v2_runtime_t* runtime,
                                                  yunlink_v2_string_view_t peer_id);
YUNLINK_V2_API uint64_t yunlink_v2_runtime_open_session(yunlink_v2_runtime_t* runtime,
                                                        yunlink_v2_string_view_t peer_id);
YUNLINK_V2_API uint16_t yunlink_v2_runtime_session_endpoint_uid(const yunlink_v2_runtime_t* runtime,
                                                                yunlink_v2_string_view_t peer_id,
                                                                uint64_t session_id,
                                                                char* out_uid,
                                                                size_t out_uid_capacity);
YUNLINK_V2_API uint16_t yunlink_v2_runtime_publish(yunlink_v2_runtime_t* runtime,
                                                   yunlink_v2_string_view_t peer_id,
                                                   uint64_t session_id,
                                                   uint8_t family,
                                                   uint8_t operation,
                                                   yunlink_v2_target_view_t target,
                                                   yunlink_v2_type_ref_view_t type_ref,
                                                   yunlink_v2_bytes_view_t payload,
                                                   uint64_t correlation_id,
                                                   uint32_t ttl_ms,
                                                   uint8_t qos_class,
                                                   yunlink_v2_string_view_t source_entity_uid,
                                                   yunlink_v2_message_handle_t* out_handle);
YUNLINK_V2_API uint64_t yunlink_v2_runtime_subscribe(yunlink_v2_runtime_t* runtime,
                                                     yunlink_v2_event_callback_t callback,
                                                     void* user_data);
YUNLINK_V2_API void yunlink_v2_runtime_unsubscribe(yunlink_v2_runtime_t* runtime, uint64_t token);
YUNLINK_V2_API uint8_t yunlink_v2_runtime_session_has_profile(const yunlink_v2_runtime_t* runtime,
                                                              yunlink_v2_string_view_t peer_id,
                                                              uint64_t session_id,
                                                              yunlink_v2_string_view_t profile_id,
                                                              uint16_t major);
YUNLINK_V2_API uint8_t
yunlink_v2_runtime_session_supports_profile(const yunlink_v2_runtime_t* runtime,
                                            yunlink_v2_string_view_t peer_id,
                                            uint64_t session_id,
                                            yunlink_v2_string_view_t profile_id,
                                            uint16_t major,
                                            uint16_t minimum_minor);

YUNLINK_V2_API uint16_t yunlink_v2_discovery_encode_query(
    uint64_t nonce,
    uint16_t response_window_ms,
    uint8_t format_version,
    yunlink_v2_string_view_t shared_secret,
    uint8_t* out_bytes,
    size_t out_capacity,
    size_t* out_size);
YUNLINK_V2_API yunlink_v2_discovery_advertisement_t*
yunlink_v2_discovery_decode_reply(yunlink_v2_bytes_view_t bytes,
                                  yunlink_v2_string_view_t shared_secret,
                                  uint64_t expected_nonce);
YUNLINK_V2_API void yunlink_v2_discovery_advertisement_destroy(
    yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API yunlink_v2_string_view_t yunlink_v2_discovery_endpoint_uid(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API yunlink_v2_string_view_t yunlink_v2_discovery_display_name(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint16_t yunlink_v2_discovery_tcp_port(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint64_t yunlink_v2_discovery_started_at_ms(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint64_t yunlink_v2_discovery_sequence(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API size_t yunlink_v2_discovery_capability_count(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API yunlink_v2_string_view_t yunlink_v2_discovery_capability_at(
    const yunlink_v2_discovery_advertisement_t* advertisement, size_t index);
YUNLINK_V2_API size_t yunlink_v2_discovery_profile_count(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint8_t yunlink_v2_discovery_profile_at(
    const yunlink_v2_discovery_advertisement_t* advertisement,
    size_t index,
    yunlink_v2_profile_view_t* out_profile);
YUNLINK_V2_API size_t yunlink_v2_discovery_attribute_count(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint8_t yunlink_v2_discovery_attribute_at(
    const yunlink_v2_discovery_advertisement_t* advertisement,
    size_t index,
    yunlink_v2_key_value_view_t* out_attribute);
YUNLINK_V2_API size_t yunlink_v2_discovery_entity_count(
    const yunlink_v2_discovery_advertisement_t* advertisement);
YUNLINK_V2_API uint8_t yunlink_v2_discovery_entity_at(
    const yunlink_v2_discovery_advertisement_t* advertisement,
    size_t index,
    yunlink_v2_discovery_entity_view_t* out_entity);
YUNLINK_V2_API size_t yunlink_v2_discovery_entity_attribute_count(
    const yunlink_v2_discovery_advertisement_t* advertisement, size_t entity_index);
YUNLINK_V2_API uint8_t yunlink_v2_discovery_entity_attribute_at(
    const yunlink_v2_discovery_advertisement_t* advertisement,
    size_t entity_index,
    size_t attribute_index,
    yunlink_v2_key_value_view_t* out_attribute);

#ifdef __cplusplus
}
#endif

#endif  // YUNLINK_C_YUNLINK_V2_H
