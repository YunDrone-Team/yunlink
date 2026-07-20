/**
 * @file include/yunlink/c/abi/types.h
 * @brief C ABI struct definitions.
 */

#ifndef YUNLINK_C_ABI_TYPES_H
#define YUNLINK_C_ABI_TYPES_H

#include "yunlink/c/abi/enums.h"

#define YUNLINK_TOPIC_LIST_BUFFER_CAPACITY 16384u
#define YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY 65536u

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
    uint32_t required_peer_capability_flags;
    const yunlink_identity_t* managed_identities;
    size_t managed_identity_count;
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

/** Complete UAV control payload used by the schema-1 Bridge. */
typedef struct yunlink_uav_control_command {
    uint8_t control_cmd;
    float desired_position_x_m;
    float desired_position_y_m;
    float desired_position_z_m;
    float desired_velocity_x_mps;
    float desired_velocity_y_mps;
    float desired_velocity_z_mps;
    float desired_acceleration_x_mps2;
    float desired_acceleration_y_mps2;
    float desired_acceleration_z_mps2;
    float desired_body_xy_position_x_m;
    float desired_body_xy_position_y_m;
    float desired_body_xy_velocity_x_mps;
    float desired_body_xy_velocity_y_mps;
    float fixed_height_m;
    uint8_t yaw_mode;
    float desired_yaw_rad;
    float desired_yaw_rate_radps;
    uint8_t controller_type;
} yunlink_uav_control_command_t;

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

typedef struct yunlink_local_odom {
    uint64_t source_stamp_ns;
    char frame_id[64];
    char child_frame_id[64];
    float x_m;
    float y_m;
    float z_m;
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    float vx_mps;
    float vy_mps;
    float vz_mps;
    float angular_x_radps;
    float angular_y_radps;
    float angular_z_radps;
} yunlink_local_odom_t;

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
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
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
    float local_orientation_x;
    float local_orientation_y;
    float local_orientation_z;
    float local_orientation_w;
} yunlink_px4_state_event_t;

typedef struct yunlink_local_odom_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
    uint64_t source_stamp_ns;
    char frame_id[64];
    char child_frame_id[64];
    float x_m;
    float y_m;
    float z_m;
    float orientation_x;
    float orientation_y;
    float orientation_z;
    float orientation_w;
    float vx_mps;
    float vy_mps;
    float vz_mps;
    float angular_x_radps;
    float angular_y_radps;
    float angular_z_radps;
} yunlink_local_odom_event_t;

typedef struct yunlink_authority_status_event {
    uint8_t state;
    uint64_t session_id;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
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

typedef struct yunlink_host_system_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint32_t source_id;
    uint64_t source_stamp_ns;
    float cpu_percent;
    float memory_percent;
    uint32_t sample_period_ms;
    char component_kind[32];
    char active_components[8192];
} yunlink_host_system_event_t;

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
    char title[128];
    char group[128];
    char description[512];
    char depends_on[1024];
    char start_preview_units[1024];
    char start_preview_commands[2048];
} yunlink_feature_get_event_t;

typedef struct yunlink_feature_start_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    char message[256];
    char feature_name[128];
} yunlink_feature_start_event_t;

typedef struct yunlink_topic_list_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    char message[256];
    char revision[128];
    /** One topic per line: name<TAB>type<TAB>publisher_count. */
    char topics[YUNLINK_TOPIC_LIST_BUFFER_CAPACITY];
} yunlink_topic_list_event_t;

typedef struct yunlink_topic_subscription_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    uint8_t subscribed;
    float max_rate_hz;
    uint32_t max_payload_bytes;
    char message[256];
    char topic_name[256];
    char type_name[256];
} yunlink_topic_subscription_event_t;

typedef struct yunlink_topic_sample_event {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
    uint64_t receive_time_ns;
    uint64_t sequence;
    uint8_t metadata_included;
    uint8_t data_truncated;
    uint32_t data_size;
    char topic_name[256];
    char type_name[256];
    char type_hash[128];
    char encoding[32];
    char message_definition[4096];
    uint8_t data[YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY];
} yunlink_topic_sample_event_t;

typedef struct yunlink_runtime_event {
    uint8_t type;
    union {
        yunlink_link_event_t link;
        yunlink_error_event_t error;
        yunlink_command_result_event_t command_result;
        yunlink_vehicle_core_state_event_t vehicle_core_state;
        yunlink_px4_state_event_t px4_state;
        yunlink_local_odom_event_t local_odom;
        yunlink_authority_status_event_t authority_status;
        yunlink_vehicle_event_data_t vehicle_event;
        yunlink_feature_list_event_t feature_list;
        yunlink_feature_get_event_t feature_get;
        yunlink_feature_start_event_t feature_start;
        yunlink_host_system_event_t host_system;
        yunlink_topic_list_event_t topic_list;
        yunlink_topic_subscription_event_t topic_subscription;
        yunlink_topic_sample_event_t topic_sample;
    } data;
} yunlink_runtime_event_t;

#include "yunlink/c/abi/configuration.h"
#include "yunlink/c/abi/managed_entities.h"
#include "yunlink/c/abi/runtime_logs.h"

#endif  // YUNLINK_C_ABI_TYPES_H
