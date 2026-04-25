/**
 * @file include/yunlink/c/yunlink_c.h
 * @brief yunlink bindings-oriented C ABI v1.
 */

#ifndef YUNLINK_C_YUNLINK_C_H
#define YUNLINK_C_YUNLINK_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#  ifdef YUNLINK_FFI_SHARED_BUILD
#    define YUNLINK_C_API __declspec(dllexport)
#  elif defined(YUNLINK_FFI_SHARED_USE)
#    define YUNLINK_C_API __declspec(dllimport)
#  else
#    define YUNLINK_C_API
#  endif
#else
#  define YUNLINK_C_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define YUNLINK_FFI_ABI_VERSION 1u

typedef struct yunlink_runtime yunlink_runtime_t;

typedef enum yunlink_result {
    YUNLINK_RESULT_OK = 0,
    YUNLINK_RESULT_INVALID_ARGUMENT = 1,
    YUNLINK_RESULT_SOCKET_ERROR = 2,
    YUNLINK_RESULT_BIND_ERROR = 3,
    YUNLINK_RESULT_LISTEN_ERROR = 4,
    YUNLINK_RESULT_CONNECT_ERROR = 5,
    YUNLINK_RESULT_TIMEOUT = 6,
    YUNLINK_RESULT_ENCODE_ERROR = 7,
    YUNLINK_RESULT_DECODE_ERROR = 8,
    YUNLINK_RESULT_CHECKSUM_MISMATCH = 9,
    YUNLINK_RESULT_INVALID_HEADER = 10,
    YUNLINK_RESULT_RUNTIME_STOPPED = 11,
    YUNLINK_RESULT_PROTOCOL_MISMATCH = 12,
    YUNLINK_RESULT_UNAUTHORIZED = 13,
    YUNLINK_RESULT_REJECTED = 14,
    YUNLINK_RESULT_INTERNAL = 15,
    YUNLINK_RESULT_NOT_FOUND = 100,
} yunlink_result_t;

typedef enum yunlink_transport {
    YUNLINK_TRANSPORT_TCP_SERVER = 1,
    YUNLINK_TRANSPORT_TCP_CLIENT = 2,
    YUNLINK_TRANSPORT_UDP_UNICAST = 3,
    YUNLINK_TRANSPORT_UDP_BROADCAST = 4,
    YUNLINK_TRANSPORT_UDP_MULTICAST = 5,
} yunlink_transport_t;

typedef enum yunlink_agent_type {
    YUNLINK_AGENT_TYPE_UNKNOWN = 0,
    YUNLINK_AGENT_TYPE_GROUND_STATION = 1,
    YUNLINK_AGENT_TYPE_UAV = 2,
    YUNLINK_AGENT_TYPE_UGV = 3,
    YUNLINK_AGENT_TYPE_SWARM_CONTROLLER = 4,
} yunlink_agent_type_t;

typedef enum yunlink_role {
    YUNLINK_ROLE_UNKNOWN = 0,
    YUNLINK_ROLE_OBSERVER = 1,
    YUNLINK_ROLE_CONTROLLER = 2,
    YUNLINK_ROLE_VEHICLE = 3,
    YUNLINK_ROLE_RELAY = 4,
} yunlink_role_t;

typedef enum yunlink_target_scope {
    YUNLINK_TARGET_SCOPE_ENTITY = 1,
    YUNLINK_TARGET_SCOPE_GROUP = 2,
    YUNLINK_TARGET_SCOPE_BROADCAST = 3,
} yunlink_target_scope_t;

typedef enum yunlink_control_source {
    YUNLINK_CONTROL_SOURCE_UNKNOWN = 0,
    YUNLINK_CONTROL_SOURCE_GROUND_STATION = 1,
    YUNLINK_CONTROL_SOURCE_REMOTE_CONTROLLER = 2,
    YUNLINK_CONTROL_SOURCE_TERMINAL = 3,
    YUNLINK_CONTROL_SOURCE_AUTONOMY = 4,
} yunlink_control_source_t;

typedef enum yunlink_authority_state {
    YUNLINK_AUTHORITY_STATE_OBSERVER = 0,
    YUNLINK_AUTHORITY_STATE_PENDING_GRANT = 1,
    YUNLINK_AUTHORITY_STATE_CONTROLLER = 2,
    YUNLINK_AUTHORITY_STATE_PREEMPTING = 3,
    YUNLINK_AUTHORITY_STATE_REVOKED = 4,
    YUNLINK_AUTHORITY_STATE_RELEASED = 5,
    YUNLINK_AUTHORITY_STATE_REJECTED = 6,
} yunlink_authority_state_t;

typedef enum yunlink_command_phase {
    YUNLINK_COMMAND_PHASE_RECEIVED = 1,
    YUNLINK_COMMAND_PHASE_ACCEPTED = 2,
    YUNLINK_COMMAND_PHASE_IN_PROGRESS = 3,
    YUNLINK_COMMAND_PHASE_SUCCEEDED = 4,
    YUNLINK_COMMAND_PHASE_FAILED = 5,
    YUNLINK_COMMAND_PHASE_CANCELLED = 6,
    YUNLINK_COMMAND_PHASE_EXPIRED = 7,
} yunlink_command_phase_t;

typedef enum yunlink_command_kind {
    YUNLINK_COMMAND_KIND_UNKNOWN = 0,
    YUNLINK_COMMAND_KIND_TAKEOFF = 1,
    YUNLINK_COMMAND_KIND_LAND = 2,
    YUNLINK_COMMAND_KIND_RETURN = 3,
    YUNLINK_COMMAND_KIND_GOTO = 4,
    YUNLINK_COMMAND_KIND_VELOCITY_SETPOINT = 5,
    YUNLINK_COMMAND_KIND_TRAJECTORY_CHUNK = 6,
    YUNLINK_COMMAND_KIND_FORMATION_TASK = 7,
} yunlink_command_kind_t;

typedef enum yunlink_session_state {
    YUNLINK_SESSION_STATE_DISCOVERED = 1,
    YUNLINK_SESSION_STATE_HANDSHAKING = 2,
    YUNLINK_SESSION_STATE_AUTHENTICATED = 3,
    YUNLINK_SESSION_STATE_NEGOTIATED = 4,
    YUNLINK_SESSION_STATE_ACTIVE = 5,
    YUNLINK_SESSION_STATE_DRAINING = 6,
    YUNLINK_SESSION_STATE_CLOSED = 7,
    YUNLINK_SESSION_STATE_LOST = 8,
    YUNLINK_SESSION_STATE_INVALID = 9,
} yunlink_session_state_t;

typedef enum yunlink_vehicle_event_kind {
    YUNLINK_VEHICLE_EVENT_KIND_INFO = 1,
    YUNLINK_VEHICLE_EVENT_KIND_TAKEOFF = 2,
    YUNLINK_VEHICLE_EVENT_KIND_LANDING = 3,
    YUNLINK_VEHICLE_EVENT_KIND_RETURN_HOME = 4,
    YUNLINK_VEHICLE_EVENT_KIND_FORMATION_UPDATE = 5,
    YUNLINK_VEHICLE_EVENT_KIND_FAULT = 6,
} yunlink_vehicle_event_kind_t;

typedef enum yunlink_runtime_event_type {
    YUNLINK_RUNTIME_EVENT_NONE = 0,
    YUNLINK_RUNTIME_EVENT_LINK = 1,
    YUNLINK_RUNTIME_EVENT_ERROR = 2,
    YUNLINK_RUNTIME_EVENT_COMMAND_RESULT = 3,
    YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE = 4,
    YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT = 5,
} yunlink_runtime_event_type_t;

typedef struct yunlink_identity {
    uint8_t agent_type;
    uint32_t agent_id;
    uint8_t role;
} yunlink_identity_t;

typedef struct yunlink_runtime_config {
    size_t struct_size;
    uint16_t udp_bind_port;
    uint16_t udp_target_port;
    uint16_t tcp_listen_port;
    int32_t connect_timeout_ms;
    int32_t io_poll_interval_ms;
    size_t max_buffer_bytes_per_peer;
    yunlink_identity_t self_identity;
    uint32_t capability_flags;
    char shared_secret[64];
    char multicast_group[64];
} yunlink_runtime_config_t;

typedef struct yunlink_peer {
    char id[128];
} yunlink_peer_t;

typedef struct yunlink_session {
    uint64_t session_id;
} yunlink_session_t;

typedef struct yunlink_session_info {
    size_t struct_size;
    uint64_t session_id;
    uint8_t state;
    yunlink_identity_t remote_identity;
    yunlink_peer_t peer;
    uint32_t capability_flags;
    char node_name[128];
} yunlink_session_info_t;

typedef struct yunlink_target_selector {
    size_t struct_size;
    uint8_t scope;
    uint8_t target_type;
    uint32_t entity_id;
    uint32_t group_id;
} yunlink_target_selector_t;

typedef struct yunlink_command_handle {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    yunlink_target_selector_t target;
} yunlink_command_handle_t;

typedef struct yunlink_takeoff_command {
    float relative_height_m;
    float max_velocity_mps;
} yunlink_takeoff_command_t;

typedef struct yunlink_land_command {
    float max_velocity_mps;
} yunlink_land_command_t;

typedef struct yunlink_return_command {
    float loiter_before_return_s;
} yunlink_return_command_t;

typedef struct yunlink_goto_command {
    float x_m;
    float y_m;
    float z_m;
    float yaw_rad;
} yunlink_goto_command_t;

typedef struct yunlink_velocity_setpoint_command {
    float vx_mps;
    float vy_mps;
    float vz_mps;
    float yaw_rate_radps;
    uint8_t body_frame;
} yunlink_velocity_setpoint_command_t;

typedef struct yunlink_vehicle_core_state {
    uint8_t armed;
    uint8_t nav_mode;
    float x_m;
    float y_m;
    float z_m;
    float vx_mps;
    float vy_mps;
    float vz_mps;
    float battery_percent;
} yunlink_vehicle_core_state_t;

typedef struct yunlink_authority_lease {
    uint8_t state;
    uint64_t session_id;
    yunlink_target_selector_t target;
    uint8_t source;
    uint32_t lease_ttl_ms;
    uint64_t expires_at_ms;
    yunlink_peer_t peer;
} yunlink_authority_lease_t;

typedef struct yunlink_link_event {
    uint8_t transport;
    uint8_t is_up;
    uint16_t peer_port;
    char peer_id[128];
    char peer_ip[64];
} yunlink_link_event_t;

typedef struct yunlink_error_event {
    uint16_t code;
    uint8_t transport;
    uint16_t peer_port;
    char peer_id[128];
    char peer_ip[64];
    char message[256];
} yunlink_error_event_t;

typedef struct yunlink_command_result_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint16_t command_kind;
    uint8_t phase;
    uint16_t result_code;
    uint8_t progress_percent;
    char detail[256];
} yunlink_command_result_event_t;

typedef struct yunlink_vehicle_core_state_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
    uint8_t armed;
    uint8_t nav_mode;
    float x_m;
    float y_m;
    float z_m;
    float vx_mps;
    float vy_mps;
    float vz_mps;
    float battery_percent;
} yunlink_vehicle_core_state_event_t;

typedef struct yunlink_vehicle_event_data {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t kind;
    uint8_t severity;
    char detail[256];
} yunlink_vehicle_event_data_t;

typedef struct yunlink_runtime_event {
    uint8_t type;
    union {
        yunlink_link_event_t link;
        yunlink_error_event_t error;
        yunlink_command_result_event_t command_result;
        yunlink_vehicle_core_state_event_t vehicle_core_state;
        yunlink_vehicle_event_data_t vehicle_event;
    } data;
} yunlink_runtime_event_t;

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

YUNLINK_C_API yunlink_result_t
yunlink_authority_request(yunlink_runtime_t* runtime,
                          const yunlink_peer_t* peer,
                          const yunlink_session_t* session,
                          const yunlink_target_selector_t* target,
                          uint8_t source,
                          uint32_t lease_ttl_ms,
                          uint8_t allow_preempt);
YUNLINK_C_API yunlink_result_t
yunlink_authority_renew(yunlink_runtime_t* runtime,
                        const yunlink_peer_t* peer,
                        const yunlink_session_t* session,
                        const yunlink_target_selector_t* target,
                        uint8_t source,
                        uint32_t lease_ttl_ms);
YUNLINK_C_API yunlink_result_t
yunlink_authority_release(yunlink_runtime_t* runtime,
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
YUNLINK_C_API yunlink_result_t
yunlink_command_publish_land(yunlink_runtime_t* runtime,
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
YUNLINK_C_API yunlink_result_t
yunlink_command_publish_goto(yunlink_runtime_t* runtime,
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

YUNLINK_C_API yunlink_result_t yunlink_runtime_poll_event(yunlink_runtime_t* runtime,
                                                          yunlink_runtime_event_t* out_event);
YUNLINK_C_API yunlink_result_t
yunlink_runtime_poll_command_result(yunlink_runtime_t* runtime,
                                    yunlink_command_result_event_t* out_event);
YUNLINK_C_API yunlink_result_t
yunlink_runtime_poll_vehicle_core_state(yunlink_runtime_t* runtime,
                                        yunlink_vehicle_core_state_event_t* out_event);

#ifdef __cplusplus
}
#endif

#endif  // YUNLINK_C_YUNLINK_C_H
