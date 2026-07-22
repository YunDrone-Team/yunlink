/** @file @brief Borrowed C ABI views for managed logical entity directories. */

#ifndef YUNLINK_C_ABI_MANAGED_ENTITIES_H
#define YUNLINK_C_ABI_MANAGED_ENTITIES_H

#include "yunlink/c/abi/configuration.h"

typedef struct yunlink_managed_entity_identity_view {
    uint8_t agent_type;
    uint32_t agent_id;
    uint8_t role;
    const uint32_t* group_ids;
    size_t group_id_count;
} yunlink_managed_entity_identity_view_t;

typedef struct yunlink_managed_entity_descriptor_view {
    yunlink_string_view_t entity_uid;
    yunlink_managed_entity_identity_view_t identity;
    yunlink_string_view_t display_name;
    yunlink_string_view_t hardware_id;
    const yunlink_string_view_t* capabilities;
    size_t capability_count;
    uint8_t availability;
} yunlink_managed_entity_descriptor_view_t;

typedef struct yunlink_managed_entity_list_response_view {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    yunlink_string_view_t message;
    yunlink_string_view_t endpoint_uid;
    yunlink_string_view_t revision;
    yunlink_managed_entity_identity_view_t primary_identity;
    const yunlink_managed_entity_descriptor_view_t* entities;
    size_t entity_count;
} yunlink_managed_entity_list_response_view_t;

typedef struct yunlink_managed_entity_directory_changed_view {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    yunlink_string_view_t endpoint_uid;
    yunlink_string_view_t revision;
} yunlink_managed_entity_directory_changed_view_t;

typedef struct yunlink_managed_entity_attachment_result_view {
    yunlink_string_view_t entity_uid;
    uint8_t accepted;
    yunlink_string_view_t message;
} yunlink_managed_entity_attachment_result_view_t;

typedef struct yunlink_managed_entity_attachment_response_view {
    uint64_t session_id;
    uint64_t message_id;
    uint64_t correlation_id;
    uint8_t success;
    yunlink_string_view_t message;
    yunlink_string_view_t endpoint_uid;
    yunlink_string_view_t directory_revision;
    const yunlink_managed_entity_attachment_result_view_t* results;
    size_t result_count;
    const yunlink_string_view_t* attached_entity_uids;
    size_t attached_entity_count;
} yunlink_managed_entity_attachment_response_view_t;

/** All nested pointers are read-only and valid only until the callback returns. */
typedef void (*yunlink_managed_entity_list_response_callback_t)(
    void* user_data,
    const yunlink_managed_entity_list_response_view_t* response);
typedef void (*yunlink_managed_entity_directory_changed_callback_t)(
    void* user_data,
    const yunlink_managed_entity_directory_changed_view_t* event);
typedef void (*yunlink_managed_entity_attachment_response_callback_t)(
    void* user_data,
    const yunlink_managed_entity_attachment_response_view_t* response);

#endif  // YUNLINK_C_ABI_MANAGED_ENTITIES_H
