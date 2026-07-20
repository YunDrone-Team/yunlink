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
    /// Source endpoint agent type.
    pub source_type: u8,
    /// Source endpoint id.
    pub source_id: u32,
    /// Source endpoint role.
    pub source_role: u8,
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
            source_type: 0,
            source_id: 0,
            source_role: 0,
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
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_px4_state_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: u8,
    pub source_id: u32,
    pub source_role: u8,
    pub connected: u8,
    pub armed: u8,
    pub flight_mode: [c_char; 32],
    pub system_status: u8,
    pub landed_state: u8,
    pub battery_voltage_v: f32,
    pub battery_current_a: f32,
    pub battery_percentage: f32,
    pub local_x_m: f32,
    pub local_y_m: f32,
    pub local_z_m: f32,
    pub local_vx_mps: f32,
    pub local_vy_mps: f32,
    pub local_vz_mps: f32,
    pub local_yaw_rad: f32,
    pub target_x_m: f32,
    pub target_y_m: f32,
    pub target_z_m: f32,
    pub target_yaw_rad: f32,
    pub target_valid: u8,
    pub local_orientation_x: f32,
    pub local_orientation_y: f32,
    pub local_orientation_z: f32,
    pub local_orientation_w: f32,
}

/// Raw local odometry runtime event.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_local_odom_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: u8,
    pub source_id: u32,
    pub source_role: u8,
    pub source_stamp_ns: u64,
    pub frame_id: [c_char; 64],
    pub child_frame_id: [c_char; 64],
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub orientation_x: f32,
    pub orientation_y: f32,
    pub orientation_z: f32,
    pub orientation_w: f32,
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    pub angular_x_radps: f32,
    pub angular_y_radps: f32,
    pub angular_z_radps: f32,
}

impl Default for yunlink_local_odom_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            source_type: 0,
            source_id: 0,
            source_role: 0,
            source_stamp_ns: 0,
            frame_id: [0; 64],
            child_frame_id: [0; 64],
            x_m: 0.0,
            y_m: 0.0,
            z_m: 0.0,
            orientation_x: 0.0,
            orientation_y: 0.0,
            orientation_z: 0.0,
            orientation_w: 1.0,
            vx_mps: 0.0,
            vy_mps: 0.0,
            vz_mps: 0.0,
            angular_x_radps: 0.0,
            angular_y_radps: 0.0,
            angular_z_radps: 0.0,
        }
    }
}

/// Raw authority lease status event payload.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_authority_status_event_t {
    /// One of the `YUNLINK_AUTHORITY_STATE_*` constants.
    pub state: u8,
    /// Session whose request, renewal, release, or revocation was reported.
    pub session_id: u64,
    pub source_type: u8,
    pub source_id: u32,
    pub source_role: u8,
    /// Granted lease lifetime in milliseconds.
    pub lease_ttl_ms: u32,
    /// Stable authority reason code.
    pub reason_code: u16,
    /// Null-terminated human-readable detail.
    pub detail: [c_char; 256],
}

impl Default for yunlink_authority_status_event_t {
    fn default() -> Self {
        Self {
            state: 0,
            session_id: 0,
            source_type: 0,
            source_id: 0,
            source_role: 0,
            lease_ttl_ms: 0,
            reason_code: 0,
            detail: [0; 256],
        }
    }
}

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

/// Raw host resource and active-component snapshot.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_host_system_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_id: u32,
    pub source_stamp_ns: u64,
    pub cpu_percent: f32,
    pub memory_percent: f32,
    pub sample_period_ms: u32,
    pub component_kind: [c_char; 32],
    pub active_components: [c_char; 8192],
}

impl Default for yunlink_host_system_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            source_id: 0,
            source_stamp_ns: 0,
            cpu_percent: 0.0,
            memory_percent: 0.0,
            sample_period_ms: 0,
            component_kind: [0; 32],
            active_components: [0; 8192],
        }
    }
}

/// C ABI union for runtime event payloads.
///
/// Callers must inspect `yunlink_runtime_event_t::type_` before reading a field
/// from this union. The safe `yunlink` crate centralizes that unsafe read in its
/// event parser and copies the active payload into an owned Rust enum.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_feature_list_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: [c_char; 256],
    pub feature_names: [c_char; 2048],
}

impl Default for yunlink_feature_list_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            success: 0,
            message: [0; 256],
            feature_names: [0; 2048],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_feature_get_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub running: u8,
    pub auto_start: u8,
    pub message: [c_char; 256],
    pub name: [c_char; 128],
    pub title: [c_char; 128],
    pub group: [c_char; 128],
    pub description: [c_char; 512],
    pub depends_on: [c_char; 1024],
    pub start_preview_units: [c_char; 1024],
    pub start_preview_commands: [c_char; 2048],
}

impl Default for yunlink_feature_get_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            success: 0,
            running: 0,
            auto_start: 0,
            message: [0; 256],
            name: [0; 128],
            title: [0; 128],
            group: [0; 128],
            description: [0; 512],
            depends_on: [0; 1024],
            start_preview_units: [0; 1024],
            start_preview_commands: [0; 2048],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_feature_start_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: [c_char; 256],
    pub feature_name: [c_char; 128],
}

impl Default for yunlink_feature_start_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            success: 0,
            message: [0; 256],
            feature_name: [0; 128],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_topic_list_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub message: [c_char; 256],
    pub revision: [c_char; 128],
    pub topics: [c_char; 16384],
}

impl Default for yunlink_topic_list_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            success: 0,
            message: [0; 256],
            revision: [0; 128],
            topics: [0; 16384],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_topic_subscription_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: u8,
    pub subscribed: u8,
    pub max_rate_hz: f32,
    pub max_payload_bytes: u32,
    pub message: [c_char; 256],
    pub topic_name: [c_char; 256],
    pub type_name: [c_char; 256],
}

impl Default for yunlink_topic_subscription_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            success: 0,
            subscribed: 0,
            max_rate_hz: 0.0,
            max_payload_bytes: 0,
            message: [0; 256],
            topic_name: [0; 256],
            type_name: [0; 256],
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_topic_sample_event_t {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: u8,
    pub source_id: u32,
    pub source_role: u8,
    pub receive_time_ns: u64,
    pub sequence: u64,
    pub metadata_included: u8,
    pub data_truncated: u8,
    pub data_size: u32,
    pub topic_name: [c_char; 256],
    pub type_name: [c_char; 256],
    pub type_hash: [c_char; 128],
    pub encoding: [c_char; 32],
    pub message_definition: [c_char; 4096],
    pub data: [u8; 65536],
}

impl Default for yunlink_topic_sample_event_t {
    fn default() -> Self {
        Self {
            session_id: 0,
            message_id: 0,
            correlation_id: 0,
            source_type: 0,
            source_id: 0,
            source_role: 0,
            receive_time_ns: 0,
            sequence: 0,
            metadata_included: 0,
            data_truncated: 0,
            data_size: 0,
            topic_name: [0; 256],
            type_name: [0; 256],
            type_hash: [0; 128],
            encoding: [0; 32],
            message_definition: [0; 4096],
            data: [0; 65536],
        }
    }
}

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
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_PX4_STATE`.
    pub px4_state: yunlink_px4_state_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_LOCAL_ODOM`.
    pub local_odom: yunlink_local_odom_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_AUTHORITY_STATUS`.
    pub authority_status: yunlink_authority_status_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT`.
    pub vehicle_event: yunlink_vehicle_event_data_t,
    pub feature_list: yunlink_feature_list_event_t,
    pub feature_get: yunlink_feature_get_event_t,
    pub feature_start: yunlink_feature_start_event_t,
    /// Active when the event type is `YUNLINK_RUNTIME_EVENT_HOST_SYSTEM`.
    pub host_system: yunlink_host_system_event_t,
    pub topic_list: yunlink_topic_list_event_t,
    pub topic_subscription: yunlink_topic_subscription_event_t,
    pub topic_sample: yunlink_topic_sample_event_t,
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
