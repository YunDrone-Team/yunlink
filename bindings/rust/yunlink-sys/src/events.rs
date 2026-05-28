use std::ffi::c_char;

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
