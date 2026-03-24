/**
 * @file include/sunraycom/c/sunraycom_c.h
 * @brief SunrayComLib C ABI 对外接口。
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

typedef struct sunraycom_runtime_config {
    uint16_t udp_bind_port;
    uint16_t udp_target_port;
    uint16_t tcp_listen_port;
    int32_t connect_timeout_ms;
    int32_t io_poll_interval_ms;
    size_t max_buffer_bytes_per_peer;
} sunraycom_runtime_config_t;

typedef enum sunraycom_event_type {
    SUNRAYCOM_EVENT_NONE = 0,
    SUNRAYCOM_EVENT_FRAME = 1,
    SUNRAYCOM_EVENT_ERROR = 2,
    SUNRAYCOM_EVENT_LINK = 3,
} sunraycom_event_type_t;

typedef struct sunraycom_event {
    sunraycom_event_type_t type;
    uint8_t transport;
    uint16_t code;
    uint8_t seq;
    uint8_t robot_id;
    uint64_t timestamp;
    uint16_t head;
    uint16_t checksum;
    uint16_t peer_port;
    char peer_id[128];
    char peer_ip[64];
    char message[256];
    uint8_t link_up;
    uint8_t payload[2048];
    size_t payload_len;
} sunraycom_event_t;

/**
 * @brief 创建 C ABI 句柄。
 * @return 成功返回有效句柄，失败返回空指针。
 */
sunraycom_handle_t* sunraycom_create(void);
/**
 * @brief 销毁 C ABI 句柄。
 * @param handle 句柄。
 */
void sunraycom_destroy(sunraycom_handle_t* handle);

/**
 * @brief 启动运行时。
 * @param handle 句柄。
 * @param cfg 运行参数，可为空（使用默认配置）。
 * @return 调用结果。
 */
sunraycom_status_t sunraycom_start(sunraycom_handle_t* handle,
                                   const sunraycom_runtime_config_t* cfg);
/**
 * @brief 停止运行时。
 * @param handle 句柄。
 */
void sunraycom_stop(sunraycom_handle_t* handle);

/**
 * @brief 发送 UDP 单播数据。
 * @param handle 句柄。
 * @param data 数据指针。
 * @param len 数据长度。
 * @param ip 目标 IP。
 * @param port 目标端口。
 * @return 发送字节数；失败返回负值。
 */
int sunraycom_send_udp_unicast(sunraycom_handle_t* handle,
                               const uint8_t* data,
                               size_t len,
                               const char* ip,
                               uint16_t port);
/**
 * @brief 发送 UDP 广播数据。
 * @param handle 句柄。
 * @param data 数据指针。
 * @param len 数据长度。
 * @param port 目标端口。
 * @return 发送字节数；失败返回负值。
 */
int sunraycom_send_udp_broadcast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port);
/**
 * @brief 发送 UDP 组播数据。
 * @param handle 句柄。
 * @param data 数据指针。
 * @param len 数据长度。
 * @param port 目标端口。
 * @return 发送字节数；失败返回负值。
 */
int sunraycom_send_udp_multicast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port);

/**
 * @brief 建立 TCP 连接。
 * @param handle 句柄。
 * @param ip 目标 IP。
 * @param port 目标端口。
 * @param out_peer_id 输出对端 ID 缓冲区。
 * @param out_peer_id_len 输出缓冲区长度。
 * @return 调用结果。
 */
sunraycom_status_t sunraycom_tcp_connect(sunraycom_handle_t* handle,
                                         const char* ip,
                                         uint16_t port,
                                         char* out_peer_id,
                                         size_t out_peer_id_len);
/**
 * @brief 向已连接 TCP 对端发送数据。
 * @param handle 句柄。
 * @param peer_id 对端 ID。
 * @param data 数据指针。
 * @param len 数据长度。
 * @return 发送字节数；失败返回负值。
 */
int sunraycom_send_tcp(sunraycom_handle_t* handle,
                       const char* peer_id,
                       const uint8_t* data,
                       size_t len);

/**
 * @brief 轮询一个事件。
 * @param handle 句柄。
 * @param out_event 输出事件。
 * @return 调用结果。
 */
sunraycom_status_t sunraycom_poll_event(sunraycom_handle_t* handle, sunraycom_event_t* out_event);

#ifdef __cplusplus
}
#endif

#endif  // SUNRAYCOM_C_SUNRAYCOM_C_H
