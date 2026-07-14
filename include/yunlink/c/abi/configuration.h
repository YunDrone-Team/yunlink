/** @file @brief Borrowed C ABI views for the configuration resource service. */

#ifndef YUNLINK_C_ABI_CONFIGURATION_H
#define YUNLINK_C_ABI_CONFIGURATION_H

#include "yunlink/c/abi/enums.h"

typedef struct yunlink_string_view {
    const char* data;
    size_t size;
} yunlink_string_view_t;

typedef struct yunlink_config_value_view {
    uint8_t type;
    uint8_t bool_value;
    int64_t int64_value;
    double double_value;
    yunlink_string_view_t string_value;
    const yunlink_string_view_t* string_list;
    size_t string_list_count;
} yunlink_config_value_view_t;

typedef struct yunlink_config_resource_descriptor_view {
    yunlink_string_view_t id;
    yunlink_string_view_t title;
    yunlink_string_view_t description;
    uint8_t readable;
    uint8_t writable;
    uint8_t apply_supported;
} yunlink_config_resource_descriptor_view_t;

typedef struct yunlink_config_choice_view {
    yunlink_config_value_view_t value;
    yunlink_string_view_t label;
} yunlink_config_choice_view_t;

typedef struct yunlink_config_field_schema_view {
    yunlink_string_view_t path;
    yunlink_string_view_t title;
    yunlink_string_view_t description;
    uint8_t type;
    uint8_t required;
    uint8_t read_only;
    uint8_t sensitive;
    uint8_t has_minimum;
    double minimum;
    uint8_t has_maximum;
    double maximum;
    yunlink_string_view_t validation_pattern;
    const yunlink_config_choice_view_t* choices;
    size_t choice_count;
} yunlink_config_field_schema_view_t;

typedef struct yunlink_config_field_value_view {
    yunlink_string_view_t path;
    yunlink_config_value_view_t value;
} yunlink_config_field_value_view_t;

typedef struct yunlink_config_snapshot_view {
    yunlink_string_view_t resource_id;
    yunlink_string_view_t revision;
    yunlink_string_view_t applied_revision;
    const yunlink_config_field_value_view_t* values;
    size_t value_count;
} yunlink_config_snapshot_view_t;

typedef struct yunlink_config_field_error_view {
    yunlink_string_view_t path;
    yunlink_string_view_t code;
    yunlink_string_view_t message;
} yunlink_config_field_error_view_t;

typedef struct yunlink_config_effects_view {
    uint8_t requirement;
    const yunlink_string_view_t* affected_components;
    size_t affected_component_count;
    uint8_t reconnect_expected;
} yunlink_config_effects_view_t;

typedef struct yunlink_configuration_handle {
    uint64_t message_id;
    uint64_t session_id;
    uint64_t created_at_ms;
    uint32_t ttl_ms;
} yunlink_configuration_handle_t;

#define YUNLINK_CONFIG_RESPONSE_HEADER                                                             \
    uint64_t session_id;                                                                           \
    uint64_t message_id;                                                                           \
    uint64_t correlation_id;                                                                       \
    uint8_t status;                                                                                \
    yunlink_string_view_t message

typedef struct yunlink_config_resource_list_response_view {
    YUNLINK_CONFIG_RESPONSE_HEADER;
    const yunlink_config_resource_descriptor_view_t* resources;
    size_t resource_count;
} yunlink_config_resource_list_response_view_t;

typedef struct yunlink_config_resource_describe_response_view {
    YUNLINK_CONFIG_RESPONSE_HEADER;
    yunlink_config_resource_descriptor_view_t resource;
    const yunlink_config_field_schema_view_t* fields;
    size_t field_count;
} yunlink_config_resource_describe_response_view_t;

typedef struct yunlink_config_resource_get_response_view {
    YUNLINK_CONFIG_RESPONSE_HEADER;
    yunlink_config_snapshot_view_t snapshot;
} yunlink_config_resource_get_response_view_t;

typedef struct yunlink_config_resource_patch_response_view {
    YUNLINK_CONFIG_RESPONSE_HEADER;
    yunlink_config_snapshot_view_t snapshot;
    const yunlink_config_field_error_view_t* errors;
    size_t error_count;
    yunlink_config_effects_view_t effects;
} yunlink_config_resource_patch_response_view_t;

typedef struct yunlink_config_resource_apply_response_view {
    YUNLINK_CONFIG_RESPONSE_HEADER;
    yunlink_string_view_t applied_revision;
    uint8_t outcome;
    yunlink_config_effects_view_t effects;
} yunlink_config_resource_apply_response_view_t;

#undef YUNLINK_CONFIG_RESPONSE_HEADER

/** All nested pointers are read-only and valid only until the callback returns. */
typedef void (*yunlink_config_resource_list_response_callback_t)(
    void* user_data,
    const yunlink_config_resource_list_response_view_t* response);
typedef void (*yunlink_config_resource_describe_response_callback_t)(
    void* user_data,
    const yunlink_config_resource_describe_response_view_t* response);
typedef void (*yunlink_config_resource_get_response_callback_t)(
    void* user_data,
    const yunlink_config_resource_get_response_view_t* response);
typedef void (*yunlink_config_resource_patch_response_callback_t)(
    void* user_data,
    const yunlink_config_resource_patch_response_view_t* response);
typedef void (*yunlink_config_resource_apply_response_callback_t)(
    void* user_data,
    const yunlink_config_resource_apply_response_view_t* response);

#endif  // YUNLINK_C_ABI_CONFIGURATION_H
