//! Owned runtime-log types and callback conversion.

mod callbacks;

use yunlink_sys as sys;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RuntimeLogSummary {
    pub runtime_id: String,
    pub feature_name: String,
    pub title: String,
    pub state: String,
    pub started_at_ns: u64,
    pub finished_at_ns: u64,
    pub exit_code: Option<i32>,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RuntimeLogListResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub runtimes: Vec<RuntimeLogSummary>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RuntimeLogReadResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub runtime_id: String,
    pub chunk: String,
    pub next_cursor: u64,
    pub truncated: bool,
    pub eof: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum RuntimeLogResponse {
    List(RuntimeLogListResponse),
    Read(RuntimeLogReadResponse),
}

pub(crate) use callbacks::{register_callbacks, RuntimeLogCallbackContext};

pub(crate) fn string_view(value: &str) -> sys::yunlink_string_view_t {
    sys::yunlink_string_view_t {
        data: value.as_ptr().cast(),
        size: value.len(),
    }
}
