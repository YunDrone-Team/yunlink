#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

use std::ffi::c_char;

pub const YUNLINK_FFI_ABI_VERSION: u32 = 1;

pub type yunlink_runtime_t = core::ffi::c_void;
pub type yunlink_result_t = i32;

pub const YUNLINK_RESULT_OK: yunlink_result_t = 0;
pub const YUNLINK_RESULT_INVALID_ARGUMENT: yunlink_result_t = 1;
pub const YUNLINK_RESULT_SOCKET_ERROR: yunlink_result_t = 2;
pub const YUNLINK_RESULT_BIND_ERROR: yunlink_result_t = 3;
pub const YUNLINK_RESULT_LISTEN_ERROR: yunlink_result_t = 4;
pub const YUNLINK_RESULT_CONNECT_ERROR: yunlink_result_t = 5;
pub const YUNLINK_RESULT_TIMEOUT: yunlink_result_t = 6;
pub const YUNLINK_RESULT_ENCODE_ERROR: yunlink_result_t = 7;
pub const YUNLINK_RESULT_DECODE_ERROR: yunlink_result_t = 8;
pub const YUNLINK_RESULT_CHECKSUM_MISMATCH: yunlink_result_t = 9;
pub const YUNLINK_RESULT_INVALID_HEADER: yunlink_result_t = 10;
pub const YUNLINK_RESULT_RUNTIME_STOPPED: yunlink_result_t = 11;
pub const YUNLINK_RESULT_PROTOCOL_MISMATCH: yunlink_result_t = 12;
pub const YUNLINK_RESULT_UNAUTHORIZED: yunlink_result_t = 13;
pub const YUNLINK_RESULT_REJECTED: yunlink_result_t = 14;
pub const YUNLINK_RESULT_INTERNAL: yunlink_result_t = 15;
pub const YUNLINK_RESULT_NOT_FOUND: yunlink_result_t = 100;

pub const YUNLINK_AGENT_TYPE_GROUND_STATION: u8 = 1;
pub const YUNLINK_AGENT_TYPE_UAV: u8 = 2;

pub const YUNLINK_ROLE_OBSERVER: u8 = 1;
pub const YUNLINK_ROLE_CONTROLLER: u8 = 2;
pub const YUNLINK_ROLE_VEHICLE: u8 = 3;

pub const YUNLINK_TARGET_SCOPE_ENTITY: u8 = 1;

pub const YUNLINK_CONTROL_SOURCE_GROUND_STATION: u8 = 1;

pub const YUNLINK_AUTHORITY_STATE_CONTROLLER: u8 = 2;

pub const YUNLINK_RUNTIME_EVENT_NONE: u8 = 0;
pub const YUNLINK_RUNTIME_EVENT_LINK: u8 = 1;
pub const YUNLINK_RUNTIME_EVENT_ERROR: u8 = 2;
pub const YUNLINK_RUNTIME_EVENT_COMMAND_RESULT: u8 = 3;
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE: u8 = 4;
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT: u8 = 5;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_identity_t {
    pub agent_type: u8,
    pub agent_id: u32,
    pub role: u8,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_runtime_config_t {
    pub struct_size: usize,
    pub udp_bind_port: u16,
    pub udp_target_port: u16,
    pub tcp_listen_port: u16,
    pub connect_timeout_ms: i32,
    pub io_poll_interval_ms: i32,
    pub max_buffer_bytes_per_peer: usize,
    pub self_identity: yunlink_identity_t,
    pub capability_flags: u32,
    pub shared_secret: [c_char; 64],
    pub multicast_group: [c_char; 64],
}

impl Default for yunlink_runtime_config_t {
    fn default() -> Self {
        Self {
            struct_size: core::mem::size_of::<Self>(),
            udp_bind_port: 0,
            udp_target_port: 0,
            tcp_listen_port: 0,
            connect_timeout_ms: 0,
            io_poll_interval_ms: 0,
            max_buffer_bytes_per_peer: 0,
            self_identity: yunlink_identity_t::default(),
            capability_flags: 0,
            shared_secret: [0; 64],
            multicast_group: [0; 64],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_peer_t {
    pub id: [c_char; 128],
}

impl Default for yunlink_peer_t {
    fn default() -> Self {
        Self { id: [0; 128] }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_session_t {
    pub session_id: u64,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_target_selector_t {
    pub struct_size: usize,
    pub scope: u8,
    pub target_type: u8,
    pub entity_id: u32,
    pub group_id: u32,
}

impl Default for yunlink_target_selector_t {
    fn default() -> Self {
        Self {
            struct_size: core::mem::size_of::<Self>(),
            scope: 0,
            target_type: 0,
            entity_id: 0,
            group_id: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_command_handle_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub target: yunlink_target_selector_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_goto_command_t {
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub yaw_rad: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_vehicle_core_state_t {
    pub armed: u8,
    pub nav_mode: u8,
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    pub battery_percent: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_authority_lease_t {
    pub state: u8,
    pub session_id: u64,
    pub target: yunlink_target_selector_t,
    pub source: u8,
    pub lease_ttl_ms: u32,
    pub expires_at_ms: u64,
    pub peer: yunlink_peer_t,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_link_event_t {
    pub transport: u8,
    pub is_up: u8,
    pub peer_port: u16,
    pub peer_id: [c_char; 128],
    pub peer_ip: [c_char; 64],
}

impl Default for yunlink_link_event_t {
    fn default() -> Self {
        Self {
            transport: 0,
            is_up: 0,
            peer_port: 0,
            peer_id: [0; 128],
            peer_ip: [0; 64],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_error_event_t {
    pub code: u16,
    pub transport: u8,
    pub peer_port: u16,
    pub peer_id: [c_char; 128],
    pub peer_ip: [c_char; 64],
    pub message: [c_char; 256],
}

impl Default for yunlink_error_event_t {
    fn default() -> Self {
        Self {
            code: 0,
            transport: 0,
            peer_port: 0,
            peer_id: [0; 128],
            peer_ip: [0; 64],
            message: [0; 256],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_command_result_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub command_kind: u16,
    pub phase: u8,
    pub result_code: u16,
    pub progress_percent: u8,
    pub detail: [c_char; 256],
}

impl Default for yunlink_command_result_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            command_kind: 0,
            phase: 0,
            result_code: 0,
            progress_percent: 0,
            detail: [0; 256],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_vehicle_core_state_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: u8,
    pub source_id: u32,
    pub source_role: u8,
    pub armed: u8,
    pub nav_mode: u8,
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    pub battery_percent: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_vehicle_event_data_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub kind: u8,
    pub severity: u8,
    pub detail: [c_char; 256],
}

impl Default for yunlink_vehicle_event_data_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            kind: 0,
            severity: 0,
            detail: [0; 256],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy)]
pub union yunlink_runtime_event_union_t {
    pub link: yunlink_link_event_t,
    pub error: yunlink_error_event_t,
    pub command_result: yunlink_command_result_event_t,
    pub vehicle_core_state: yunlink_vehicle_core_state_event_t,
    pub vehicle_event: yunlink_vehicle_event_data_t,
}

impl Default for yunlink_runtime_event_union_t {
    fn default() -> Self {
        Self {
            link: yunlink_link_event_t::default(),
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct yunlink_runtime_event_t {
    pub type_: u8,
    pub data: yunlink_runtime_event_union_t,
}

unsafe extern "C" {
    pub fn yunlink_ffi_abi_version() -> u32;
    pub fn yunlink_result_name(result: yunlink_result_t) -> *const c_char;

    pub fn yunlink_runtime_create(out_runtime: *mut *mut yunlink_runtime_t) -> yunlink_result_t;
    pub fn yunlink_runtime_destroy(runtime: *mut yunlink_runtime_t);
    pub fn yunlink_runtime_start(
        runtime: *mut yunlink_runtime_t,
        cfg: *const yunlink_runtime_config_t,
    ) -> yunlink_result_t;
    pub fn yunlink_runtime_stop(runtime: *mut yunlink_runtime_t) -> yunlink_result_t;

    pub fn yunlink_peer_connect(
        runtime: *mut yunlink_runtime_t,
        ip: *const c_char,
        port: u16,
        out_peer: *mut yunlink_peer_t,
    ) -> yunlink_result_t;
    pub fn yunlink_session_open(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        node_name: *const c_char,
        out_session: *mut yunlink_session_t,
    ) -> yunlink_result_t;

    pub fn yunlink_authority_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
        allow_preempt: u8,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_renew(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_release(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_current(
        runtime: *mut yunlink_runtime_t,
        out_lease: *mut yunlink_authority_lease_t,
    ) -> yunlink_result_t;

    pub fn yunlink_command_publish_goto(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_goto_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;

    pub fn yunlink_publish_vehicle_core_state(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_vehicle_core_state_t,
        session_id: u64,
    ) -> yunlink_result_t;

    pub fn yunlink_runtime_poll_event(
        runtime: *mut yunlink_runtime_t,
        out_event: *mut yunlink_runtime_event_t,
    ) -> yunlink_result_t;
}
