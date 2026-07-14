//! Raw borrowed configuration resource views.

use std::ffi::c_char;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_string_view_t {
    pub data: *const c_char,
    pub size: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_value_view_t {
    pub type_: u8,
    pub bool_value: u8,
    pub int64_value: i64,
    pub double_value: f64,
    pub string_value: yunlink_string_view_t,
    pub string_list: *const yunlink_string_view_t,
    pub string_list_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_resource_descriptor_view_t {
    pub id: yunlink_string_view_t,
    pub title: yunlink_string_view_t,
    pub description: yunlink_string_view_t,
    pub readable: u8,
    pub writable: u8,
    pub apply_supported: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_choice_view_t {
    pub value: yunlink_config_value_view_t,
    pub label: yunlink_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_field_schema_view_t {
    pub path: yunlink_string_view_t,
    pub title: yunlink_string_view_t,
    pub description: yunlink_string_view_t,
    pub type_: u8,
    pub required: u8,
    pub read_only: u8,
    pub sensitive: u8,
    pub has_minimum: u8,
    pub minimum: f64,
    pub has_maximum: u8,
    pub maximum: f64,
    pub validation_pattern: yunlink_string_view_t,
    pub choices: *const yunlink_config_choice_view_t,
    pub choice_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_field_value_view_t {
    pub path: yunlink_string_view_t,
    pub value: yunlink_config_value_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_snapshot_view_t {
    pub resource_id: yunlink_string_view_t,
    pub revision: yunlink_string_view_t,
    pub applied_revision: yunlink_string_view_t,
    pub values: *const yunlink_config_field_value_view_t,
    pub value_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_field_error_view_t {
    pub path: yunlink_string_view_t,
    pub code: yunlink_string_view_t,
    pub message: yunlink_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_config_effects_view_t {
    pub requirement: u8,
    pub affected_components: *const yunlink_string_view_t,
    pub affected_component_count: usize,
    pub reconnect_expected: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_configuration_handle_t {
    pub message_id: u64,
    pub session_id: u64,
    pub created_at_ms: u64,
    pub ttl_ms: u32,
}

macro_rules! response_view {
    ($name:ident { $($field:ident: $type:ty),* $(,)? }) => {
        #[repr(C)]
        #[derive(Clone, Copy, Debug, Default)]
        pub struct $name {
            pub session_id: u64,
            pub message_id: u64,
            pub correlation_id: u64,
            pub status: u8,
            pub message: yunlink_string_view_t,
            $(pub $field: $type,)*
        }
    };
}

response_view!(yunlink_config_resource_list_response_view_t {
    resources: *const yunlink_config_resource_descriptor_view_t,
    resource_count: usize,
});
response_view!(yunlink_config_resource_describe_response_view_t {
    resource: yunlink_config_resource_descriptor_view_t,
    fields: *const yunlink_config_field_schema_view_t,
    field_count: usize,
});
response_view!(yunlink_config_resource_get_response_view_t {
    snapshot: yunlink_config_snapshot_view_t,
});
response_view!(yunlink_config_resource_patch_response_view_t {
    snapshot: yunlink_config_snapshot_view_t,
    errors: *const yunlink_config_field_error_view_t,
    error_count: usize,
    effects: yunlink_config_effects_view_t,
});
response_view!(yunlink_config_resource_apply_response_view_t {
    applied_revision: yunlink_string_view_t,
    outcome: u8,
    effects: yunlink_config_effects_view_t,
});

pub type yunlink_config_resource_list_response_callback_t = Option<
    unsafe extern "C" fn(
        *mut core::ffi::c_void,
        *const yunlink_config_resource_list_response_view_t,
    ),
>;
pub type yunlink_config_resource_describe_response_callback_t = Option<
    unsafe extern "C" fn(
        *mut core::ffi::c_void,
        *const yunlink_config_resource_describe_response_view_t,
    ),
>;
pub type yunlink_config_resource_get_response_callback_t = Option<
    unsafe extern "C" fn(
        *mut core::ffi::c_void,
        *const yunlink_config_resource_get_response_view_t,
    ),
>;
pub type yunlink_config_resource_patch_response_callback_t = Option<
    unsafe extern "C" fn(
        *mut core::ffi::c_void,
        *const yunlink_config_resource_patch_response_view_t,
    ),
>;
pub type yunlink_config_resource_apply_response_callback_t = Option<
    unsafe extern "C" fn(
        *mut core::ffi::c_void,
        *const yunlink_config_resource_apply_response_view_t,
    ),
>;
