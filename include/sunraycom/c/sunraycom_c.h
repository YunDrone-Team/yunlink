/**
 * @file include/sunraycom/c/sunraycom_c.h
 * @brief sunray_communication_lib C ABI 对外接口。
 */

#ifndef SUNRAYCOM_C_SUNRAYCOM_C_H
#define SUNRAYCOM_C_SUNRAYCOM_C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sunraycom_handle sunraycom_handle_t;

typedef enum sunraycom_status {
    SUNRAYCOM_STATUS_OK = 0,
    SUNRAYCOM_STATUS_ERR = 1,
} sunraycom_status_t;

typedef struct sunraycom_identity {
    uint8_t agent_type;
    uint32_t agent_id;
    uint8_t role;
} sunraycom_identity_t;

typedef struct sunraycom_runtime_config {
    uint16_t udp_bind_port;
    uint16_t udp_target_port;
    uint16_t tcp_listen_port;
    int32_t connect_timeout_ms;
    int32_t io_poll_interval_ms;
    size_t max_buffer_bytes_per_peer;
    sunraycom_identity_t self_identity;
    uint32_t capability_flags;
    char shared_secret[64];
} sunraycom_runtime_config_t;

typedef enum sunraycom_event_type {
    SUNRAYCOM_EVENT_NONE = 0,
    SUNRAYCOM_EVENT_ENVELOPE = 1,
    SUNRAYCOM_EVENT_ERROR = 2,
    SUNRAYCOM_EVENT_LINK = 3,
} sunraycom_event_type_t;

typedef struct sunraycom_event {
    sunraycom_event_type_t type;
    uint8_t transport;
    uint16_t code;
    uint8_t protocol_major;
    uint8_t qos_class;
    uint8_t message_family;
    uint16_t message_type;
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint64_t created_at_ms;
    uint32_t ttl_ms;
    uint32_t checksum;
    uint8_t source_type;
    uint32_t source_id;
    uint8_t source_role;
    uint8_t target_scope;
    uint8_t target_type;
    uint32_t target_group_id;
    uint16_t peer_port;
    char peer_id[128];
    char peer_ip[64];
    char message[256];
    uint8_t link_up;
    uint8_t payload[2048];
    size_t payload_len;
} sunraycom_event_t;

sunraycom_handle_t* sunraycom_create(void);
void sunraycom_destroy(sunraycom_handle_t* handle);

sunraycom_status_t sunraycom_start(sunraycom_handle_t* handle,
                                   const sunraycom_runtime_config_t* cfg);
void sunraycom_stop(sunraycom_handle_t* handle);

int sunraycom_send_udp_unicast(sunraycom_handle_t* handle,
                               const uint8_t* data,
                               size_t len,
                               const char* ip,
                               uint16_t port);
int sunraycom_send_udp_broadcast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port);
int sunraycom_send_udp_multicast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port);

sunraycom_status_t sunraycom_tcp_connect(sunraycom_handle_t* handle,
                                         const char* ip,
                                         uint16_t port,
                                         char* out_peer_id,
                                         size_t out_peer_id_len);
int sunraycom_send_tcp(sunraycom_handle_t* handle,
                       const char* peer_id,
                       const uint8_t* data,
                       size_t len);

sunraycom_status_t sunraycom_poll_event(sunraycom_handle_t* handle, sunraycom_event_t* out_event);

#ifdef __cplusplus
}
#endif

#endif  // SUNRAYCOM_C_SUNRAYCOM_C_H
