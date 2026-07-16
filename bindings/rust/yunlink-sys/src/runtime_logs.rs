//! Raw borrowed runtime-log response views.

use crate::configuration::yunlink_string_view_t;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_runtime_log_summary_view_t {
    pub runtime_id: yunlink_string_view_t,
    pub feature_name: yunlink_string_view_t,
    pub title: yunlink_string_view_t,
    pub state: yunlink_string_view_t,
    pub started_at_ns: u64,
    pub finished_at_ns: u64,
    pub has_exit_code: u8,
    pub exit_code: i32,
    pub message: yunlink_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_runtime_log_list_response_view_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: yunlink_string_view_t,
    pub runtimes: *const yunlink_runtime_log_summary_view_t,
    pub runtime_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_runtime_log_read_response_view_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: yunlink_string_view_t,
    pub runtime_id: yunlink_string_view_t,
    pub chunk: yunlink_string_view_t,
    pub next_cursor: u64,
    pub truncated: u8,
    pub eof: u8,
}

pub type yunlink_runtime_log_list_response_callback_t = Option<
    unsafe extern "C" fn(*mut core::ffi::c_void, *const yunlink_runtime_log_list_response_view_t),
>;
pub type yunlink_runtime_log_read_response_callback_t = Option<
    unsafe extern "C" fn(*mut core::ffi::c_void, *const yunlink_runtime_log_read_response_view_t),
>;
