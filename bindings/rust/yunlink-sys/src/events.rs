use std::ffi::c_char;

/// Raw link-state runtime event.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_link_event_t {
    /// One of the transport constants from the C ABI.
    pub transport: u8,
    /// Non-zero when the link is up.
    pub is_up: u8,
    /// Remote peer port.
    pub peer_port: u16,
    /// Null-terminated peer id buffer.
    pub peer_id: [c_char; 128],
    /// Null-terminated peer IP buffer.
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

/// Raw runtime error event.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_error_event_t {
    /// Stable runtime or protocol error code.
    pub code: u16,
    /// Transport that produced the error, if known.
    pub transport: u8,
    /// Remote peer port, if known.
    pub peer_port: u16,
    /// Null-terminated peer id buffer.
    pub peer_id: [c_char; 128],
    /// Null-terminated peer IP buffer.
    pub peer_ip: [c_char; 64],
    /// Null-terminated human-readable error message.
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

/// Raw command execution result event.
///
/// This event is intentionally metadata-rich so a UI can match command
/// lifecycle updates to a previously returned `yunlink_command_handle_t`.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_command_result_event_t {
    /// Session that produced the result.
    pub session_id: u64,
    /// Result message id.
    pub message_id: u64,
    /// Correlation id copied from the original command handle.
    pub correlation_id: u64,
    /// One of the `YUNLINK_COMMAND_KIND_*` constants.
    pub command_kind: u16,
    /// One of the `YUNLINK_COMMAND_PHASE_*` constants.
    pub phase: u8,
    /// Stable result code for the command execution.
    pub result_code: u16,
    /// Progress percentage when the executor reports partial progress.
    pub progress_percent: u8,
    /// Null-terminated human-readable detail buffer.
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

/// Raw vehicle core state event payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_vehicle_core_state_event_t {
    /// Session carrying the state.
    pub session_id: u64,
    /// State message id.
    pub message_id: u64,
    /// State correlation id.
    pub correlation_id: u64,
    /// Source endpoint agent type.
    pub source_type: u8,
    /// Source endpoint id.
    pub source_id: u32,
    /// Source endpoint role.
    pub source_role: u8,
    /// Non-zero when the vehicle is armed.
    pub armed: u8,
    /// Vehicle navigation mode as a protocol-defined integer.
    pub nav_mode: u8,
    /// Position X in meters.
    pub x_m: f32,
    /// Position Y in meters.
    pub y_m: f32,
    /// Position Z in meters.
    pub z_m: f32,
    /// Velocity X in meters per second.
    pub vx_mps: f32,
    /// Velocity Y in meters per second.
    pub vy_mps: f32,
    /// Velocity Z in meters per second.
    pub vz_mps: f32,
    /// Battery percentage, normally in the range 0..=100.
    pub battery_percent: f32,
}

/// Raw higher-level vehicle event payload.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_vehicle_event_data_t {
    /// Session carrying the event.
    pub session_id: u64,
    /// Event message id.
    pub message_id: u64,
    /// Event correlation id.
    pub correlation_id: u64,
    /// Event kind from the C ABI vehicle-event constants.
    pub kind: u8,
    /// Event severity as a protocol-defined integer.
    pub severity: u8,
    /// Null-terminated human-readable event detail.
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

/// C ABI union for runtime event payloads.
///
/// Callers must inspect `yunlink_runtime_event_t::type_` before reading a field
/// from this union. The safe `yunlink` crate centralizes that unsafe read in its
/// event parser and copies the active payload into an owned Rust enum.
#[repr(C)]
#[derive(Clone, Copy)]
pub union yunlink_runtime_event_union_t {
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_LINK`.
    pub link: yunlink_link_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_ERROR`.
    pub error: yunlink_error_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_COMMAND_RESULT`.
    pub command_result: yunlink_command_result_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE`.
    pub vehicle_core_state: yunlink_vehicle_core_state_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT`.
    pub vehicle_event: yunlink_vehicle_event_data_t,
}

impl Default for yunlink_runtime_event_union_t {
    fn default() -> Self {
        // Any union field is valid as a zeroed default; the event type decides
        // which field may be read after polling.
        Self {
            link: yunlink_link_event_t::default(),
        }
    }
}

/// Raw tagged runtime event returned by `yunlink_runtime_poll_event`.
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct yunlink_runtime_event_t {
    /// One of the `YUNLINK_RUNTIME_EVENT_*` constants.
    pub type_: u8,
    /// Payload union selected by `type_`.
    pub data: yunlink_runtime_event_union_t,
}
