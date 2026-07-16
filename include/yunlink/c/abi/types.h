/**
 * @file include/yunlink/c/abi/types.h
 * @brief C ABI struct definitions.
 */

#ifndef YUNLINK_C_ABI_TYPES_H
#define YUNLINK_C_ABI_TYPES_H

#include "yunlink/c/abi/enums.h"

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
    uint8_t qos_profile;
    uint8_t qos_reliable_ordered_transport;
    uint8_t qos_reliable_latest_transport;
    uint8_t qos_best_effort_transport;
    uint8_t qos_bulk_transport;
    uint8_t qos_udp_fallback_to_tcp;
    uint32_t security_key_epoch;
    uint8_t security_tags_enabled;
    uint8_t security_tags_required;
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
    uint8_t reserved;
} yunlink_takeoff_command_t;

typedef struct yunlink_land_command {
    uint8_t reserved;
} yunlink_land_command_t;

typedef struct yunlink_return_command {
    uint8_t reserved;
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

typedef struct yunlink_px4_state_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
    uint8_t connected;
    uint8_t armed;
    char flight_mode[32];
    uint8_t system_status;
    uint8_t landed_state;
    float battery_voltage_v;
    float battery_current_a;
    float battery_percentage;
    float local_x_m;
    float local_y_m;
    float local_z_m;
    float local_vx_mps;
    float local_vy_mps;
    float local_vz_mps;
    float local_yaw_rad;
    float target_x_m;
    float target_y_m;
    float target_z_m;
    float target_yaw_rad;
    uint8_t target_valid;
} yunlink_px4_state_event_t;

typedef struct yunlink_authority_status_event {
    uint8_t state;
    uint64_t session_id;
    uint32_t lease_ttl_ms;
    uint16_t reason_code;
    char detail[256];
} yunlink_authority_status_event_t;

typedef struct yunlink_vehicle_event_data {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t kind;
    uint8_t severity;
    char detail[256];
} yunlink_vehicle_event_data_t;

typedef struct yunlink_feature_list_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    char message[256];
    char feature_names[2048];
} yunlink_feature_list_event_t;

typedef struct yunlink_feature_get_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    uint8_t running;
    uint8_t auto_start;
    char message[256];
    char name[128];
    char group[128];
    char description[512];
    char depends_on[1024];
    char start_preview_units[1024];
    char start_preview_commands[2048];
} yunlink_feature_get_event_t;

typedef struct yunlink_runtime_event {
    uint8_t type;
    union {
        yunlink_link_event_t link;
        yunlink_error_event_t error;
        yunlink_command_result_event_t command_result;
        yunlink_vehicle_core_state_event_t vehicle_core_state;
        yunlink_px4_state_event_t px4_state;
        yunlink_authority_status_event_t authority_status;
        yunlink_vehicle_event_data_t vehicle_event;
        yunlink_feature_list_event_t feature_list;
        yunlink_feature_get_event_t feature_get;
    } data;
} yunlink_runtime_event_t;

#include "yunlink/c/abi/configuration.h"
#include "yunlink/c/abi/runtime_logs.h"

#endif  // YUNLINK_C_ABI_TYPES_H
