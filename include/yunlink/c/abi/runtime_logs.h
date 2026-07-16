/** @file @brief Borrowed C ABI views for managed runtime log access. */

#ifndef YUNLINK_C_ABI_RUNTIME_LOGS_H
#define YUNLINK_C_ABI_RUNTIME_LOGS_H

#include "yunlink/c/abi/configuration.h"

typedef struct yunlink_runtime_log_summary_view {
    yunlink_string_view_t runtime_id;
    yunlink_string_view_t feature_name;
    yunlink_string_view_t title;
    yunlink_string_view_t state;
    uint64_t started_at_ns;
    uint64_t finished_at_ns;
    uint8_t has_exit_code;
    int32_t exit_code;
    yunlink_string_view_t message;
} yunlink_runtime_log_summary_view_t;

typedef struct yunlink_runtime_log_list_response_view {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    yunlink_string_view_t message;
    const yunlink_runtime_log_summary_view_t* runtimes;
    size_t runtime_count;
} yunlink_runtime_log_list_response_view_t;

typedef struct yunlink_runtime_log_read_response_view {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    yunlink_string_view_t message;
    yunlink_string_view_t runtime_id;
    yunlink_string_view_t chunk;
    uint64_t next_cursor;
    uint8_t truncated;
    uint8_t eof;
} yunlink_runtime_log_read_response_view_t;

/** All nested pointers are read-only and valid only until the callback returns. */
typedef void (*yunlink_runtime_log_list_response_callback_t)(
    void* user_data,
    const yunlink_runtime_log_list_response_view_t* response);
typedef void (*yunlink_runtime_log_read_response_callback_t)(
    void* user_data,
    const yunlink_runtime_log_read_response_view_t* response);

#endif  // YUNLINK_C_ABI_RUNTIME_LOGS_H
