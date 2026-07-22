use std::ffi::c_void;

use crate::configuration::yunlink_string_view_t;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_managed_entity_identity_view_t {
    pub agent_type: u8,
    pub agent_id: u32,
    pub role: u8,
    pub group_ids: *const u32,
    pub group_id_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_managed_entity_descriptor_view_t {
    pub entity_uid: yunlink_string_view_t,
    pub identity: yunlink_managed_entity_identity_view_t,
    pub display_name: yunlink_string_view_t,
    pub hardware_id: yunlink_string_view_t,
    pub capabilities: *const yunlink_string_view_t,
    pub capability_count: usize,
    pub availability: u8,
}

impl Default for yunlink_managed_entity_descriptor_view_t {
    fn default() -> Self {
        Self {
            entity_uid: yunlink_string_view_t::default(),
            identity: yunlink_managed_entity_identity_view_t::default(),
            display_name: yunlink_string_view_t::default(),
            hardware_id: yunlink_string_view_t::default(),
            capabilities: std::ptr::null(),
            capability_count: 0,
            availability: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_managed_entity_list_response_view_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: yunlink_string_view_t,
    pub endpoint_uid: yunlink_string_view_t,
    pub revision: yunlink_string_view_t,
    pub primary_identity: yunlink_managed_entity_identity_view_t,
    pub entities: *const yunlink_managed_entity_descriptor_view_t,
    pub entity_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_managed_entity_directory_changed_view_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub endpoint_uid: yunlink_string_view_t,
    pub revision: yunlink_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_managed_entity_attachment_result_view_t {
    pub entity_uid: yunlink_string_view_t,
    pub accepted: u8,
    pub message: yunlink_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_managed_entity_attachment_response_view_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: yunlink_string_view_t,
    pub endpoint_uid: yunlink_string_view_t,
    pub directory_revision: yunlink_string_view_t,
    pub results: *const yunlink_managed_entity_attachment_result_view_t,
    pub result_count: usize,
    pub attached_entity_uids: *const yunlink_string_view_t,
    pub attached_entity_count: usize,
}

pub type yunlink_managed_entity_list_response_callback_t =
    Option<unsafe extern "C" fn(*mut c_void, *const yunlink_managed_entity_list_response_view_t)>;
pub type yunlink_managed_entity_directory_changed_callback_t = Option<
    unsafe extern "C" fn(*mut c_void, *const yunlink_managed_entity_directory_changed_view_t),
>;
pub type yunlink_managed_entity_attachment_response_callback_t = Option<
    unsafe extern "C" fn(*mut c_void, *const yunlink_managed_entity_attachment_response_view_t),
>;
