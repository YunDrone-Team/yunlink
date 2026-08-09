use std::ffi::{c_char, c_void};

pub const YUNLINK_V2_ABI_VERSION: u32 = 2;

#[repr(C)]
pub struct yunlink_v2_runtime_t {
    _private: [u8; 0],
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct yunlink_v2_string_view_t {
    pub data: *const c_char,
    pub len: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct yunlink_v2_bytes_view_t {
    pub data: *const u8,
    pub len: usize,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct yunlink_v2_profile_view_t {
    pub profile_id: yunlink_v2_string_view_t,
    pub major: u16,
    pub minor: u16,
    pub schema_digest: yunlink_v2_string_view_t,
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct yunlink_v2_type_ref_view_t {
    pub profile_id: yunlink_v2_string_view_t,
    pub major: u16,
    pub minor: u16,
    pub type_name: yunlink_v2_string_view_t,
}

#[repr(C)]
pub struct yunlink_v2_runtime_config_t {
    pub struct_size: usize,
    pub endpoint_uid: yunlink_v2_string_view_t,
    pub display_name: yunlink_v2_string_view_t,
    pub shared_secret: yunlink_v2_string_view_t,
    pub tcp_listen_port: u16,
    pub profiles: *const yunlink_v2_profile_view_t,
    pub profile_count: usize,
    pub required_profiles: *const yunlink_v2_profile_view_t,
    pub required_profile_count: usize,
}

#[repr(C)]
pub struct yunlink_v2_peer_t {
    pub id: [c_char; 256],
    pub ip: [c_char; 64],
    pub port: u16,
}

impl Default for yunlink_v2_peer_t {
    fn default() -> Self {
        Self {
            id: [0; 256],
            ip: [0; 64],
            port: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct yunlink_v2_target_view_t {
    pub scope: u8,
    pub uids: *const yunlink_v2_string_view_t,
    pub uid_count: usize,
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct yunlink_v2_message_handle_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
}

#[repr(C)]
pub struct yunlink_v2_event_t {
    pub kind: u8,
    pub peer_id: yunlink_v2_string_view_t,
    pub link_up: u8,
    pub error_code: u16,
    pub message: yunlink_v2_string_view_t,
    pub session_state: u8,
    pub session_authenticated: u8,
    pub session_id: u64,
    pub family: u8,
    pub operation: u8,
    pub qos_class: u8,
    pub message_id: u64,
    pub correlation_id: u64,
    pub created_at_ms: u64,
    pub ttl_ms: u32,
    pub source_endpoint_uid: yunlink_v2_string_view_t,
    pub source_entity_uid: yunlink_v2_string_view_t,
    pub target: yunlink_v2_target_view_t,
    pub type_ref: yunlink_v2_type_ref_view_t,
    pub payload: yunlink_v2_bytes_view_t,
}

pub type yunlink_v2_event_callback_t =
    Option<unsafe extern "C" fn(*const yunlink_v2_event_t, *mut c_void)>;

extern "C" {
    pub fn yunlink_v2_abi_version() -> u32;
    pub fn yunlink_v2_runtime_create() -> *mut yunlink_v2_runtime_t;
    pub fn yunlink_v2_runtime_destroy(runtime: *mut yunlink_v2_runtime_t);
    pub fn yunlink_v2_runtime_start(
        runtime: *mut yunlink_v2_runtime_t,
        config: *const yunlink_v2_runtime_config_t,
    ) -> u16;
    pub fn yunlink_v2_runtime_stop(runtime: *mut yunlink_v2_runtime_t);
    pub fn yunlink_v2_runtime_connect(
        runtime: *mut yunlink_v2_runtime_t,
        ip: yunlink_v2_string_view_t,
        port: u16,
        out_peer: *mut yunlink_v2_peer_t,
    ) -> u16;
    pub fn yunlink_v2_runtime_close_peer(
        runtime: *mut yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
    );
    pub fn yunlink_v2_runtime_open_session(
        runtime: *mut yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
    ) -> u64;
    pub fn yunlink_v2_runtime_session_endpoint_uid(
        runtime: *const yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
        session_id: u64,
        out_uid: *mut c_char,
        out_uid_capacity: usize,
    ) -> u16;
    pub fn yunlink_v2_runtime_publish(
        runtime: *mut yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
        session_id: u64,
        family: u8,
        operation: u8,
        target: yunlink_v2_target_view_t,
        type_ref: yunlink_v2_type_ref_view_t,
        payload: yunlink_v2_bytes_view_t,
        correlation_id: u64,
        ttl_ms: u32,
        qos_class: u8,
        source_entity_uid: yunlink_v2_string_view_t,
        out_handle: *mut yunlink_v2_message_handle_t,
    ) -> u16;
    pub fn yunlink_v2_runtime_subscribe(
        runtime: *mut yunlink_v2_runtime_t,
        callback: yunlink_v2_event_callback_t,
        user_data: *mut c_void,
    ) -> u64;
    pub fn yunlink_v2_runtime_unsubscribe(runtime: *mut yunlink_v2_runtime_t, token: u64);
    pub fn yunlink_v2_runtime_session_has_profile(
        runtime: *const yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
        session_id: u64,
        profile_id: yunlink_v2_string_view_t,
        major: u16,
    ) -> u8;
    pub fn yunlink_v2_runtime_session_supports_profile(
        runtime: *const yunlink_v2_runtime_t,
        peer_id: yunlink_v2_string_view_t,
        session_id: u64,
        profile_id: yunlink_v2_string_view_t,
        major: u16,
        minimum_minor: u16,
    ) -> u8;
}
