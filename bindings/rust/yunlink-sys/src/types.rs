use std::ffi::c_char;

/// Opaque runtime handle owned by the C++ core.
///
/// Rust never inspects the pointed-to memory. Safe wrappers create and destroy
/// this value only through `yunlink_runtime_create` and `yunlink_runtime_destroy`.
pub type yunlink_runtime_t = core::ffi::c_void;

/// Raw endpoint identity passed across the C ABI.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_identity_t {
    /// One of the `YUNLINK_AGENT_TYPE_*` constants.
    pub agent_type: u8,
    /// User- or system-assigned endpoint id.
    pub agent_id: u32,
    /// One of the `YUNLINK_ROLE_*` constants.
    pub role: u8,
}

/// Raw runtime startup configuration.
///
/// `struct_size` must be initialized by callers before crossing the ABI. The C++
/// side uses it as a versioning guard so newer fields can be appended without
/// changing existing function signatures.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_runtime_config_t {
    /// Size of this struct as seen by the caller.
    pub struct_size: usize,
    /// UDP port bound by the local runtime.
    pub udp_bind_port: u16,
    /// UDP port targeted for outgoing datagrams.
    pub udp_target_port: u16,
    /// TCP server listen port.
    pub tcp_listen_port: u16,
    /// Connect timeout in milliseconds.
    pub connect_timeout_ms: i32,
    /// Runtime IO polling interval in milliseconds.
    pub io_poll_interval_ms: i32,
    /// Per-peer buffer cap used by the runtime.
    pub max_buffer_bytes_per_peer: usize,
    /// Local endpoint identity announced during session setup.
    pub self_identity: yunlink_identity_t,
    /// Feature flags advertised by the endpoint.
    pub capability_flags: u32,
    /// Null-terminated fixed buffer containing the shared secret.
    pub shared_secret: [c_char; 64],
    /// Null-terminated fixed buffer containing the multicast group.
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

/// Raw peer handle.
///
/// The C ABI keeps the identifier in a fixed buffer so ownership does not cross
/// the language boundary.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_peer_t {
    /// Null-terminated peer id string.
    pub id: [c_char; 128],
}

impl Default for yunlink_peer_t {
    fn default() -> Self {
        Self { id: [0; 128] }
    }
}

/// Raw session handle returned by `yunlink_session_open`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_session_t {
    /// Stable session id assigned by the runtime.
    pub session_id: u64,
}

/// Raw session description returned by `yunlink_session_describe`.
///
/// Like other extensible ABI structs, callers set `struct_size` before the call
/// so the C++ side can populate only the fields known to that caller.
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_session_info_t {
    /// Size of this struct as seen by the caller.
    pub struct_size: usize,
    /// Described session id.
    pub session_id: u64,
    /// One of the `YUNLINK_SESSION_STATE_*` constants.
    pub state: u8,
    /// Identity of the remote endpoint.
    pub remote_identity: yunlink_identity_t,
    /// Peer associated with the session.
    pub peer: yunlink_peer_t,
    /// Negotiated capability flags.
    pub capability_flags: u32,
    /// Null-terminated remote node name.
    pub node_name: [c_char; 128],
}

impl Default for yunlink_session_info_t {
    fn default() -> Self {
        Self {
            struct_size: core::mem::size_of::<Self>(),
            session_id: 0,
            state: 0,
            remote_identity: yunlink_identity_t::default(),
            peer: yunlink_peer_t::default(),
            capability_flags: 0,
            node_name: [0; 128],
        }
    }
}

/// Raw target selector used by authority and command APIs.
///
/// This is the C ABI form of a Rust target expression such as "UAV #1" or
/// "broadcast to all UAVs".
#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct yunlink_target_selector_t {
    /// Size of this struct as seen by the caller.
    pub struct_size: usize,
    /// One of the `YUNLINK_TARGET_SCOPE_*` constants.
    pub scope: u8,
    /// Endpoint type selected by the target.
    pub target_type: u8,
    /// Concrete entity id when `scope` is entity.
    pub entity_id: u32,
    /// Group id when `scope` is group.
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

/// Raw command handle returned by command publish functions.
///
/// The `correlation_id` is the value UI and tests use to connect a sent command
/// to a later `yunlink_command_result_event_t`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_command_handle_t {
    /// Session that published the command.
    pub session_id: u64,
    /// Protocol message id assigned to the command.
    pub message_id: u64,
    /// Correlation id echoed by command result events.
    pub correlation_id: u64,
    /// Target selector copied from the publish request.
    pub target: yunlink_target_selector_t,
}

/// Raw takeoff command payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_takeoff_command_t {
    /// Relative takeoff height in meters.
    pub relative_height_m: f32,
    /// Maximum allowed velocity in meters per second.
    pub max_velocity_mps: f32,
}

/// Raw land command payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_land_command_t {
    /// Maximum allowed velocity in meters per second.
    pub max_velocity_mps: f32,
}

/// Raw return-to-home command payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_return_command_t {
    /// Loiter time before returning, in seconds.
    pub loiter_before_return_s: f32,
}

/// Raw goto command payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_goto_command_t {
    /// Target X coordinate in meters.
    pub x_m: f32,
    /// Target Y coordinate in meters.
    pub y_m: f32,
    /// Target Z coordinate in meters.
    pub z_m: f32,
    /// Target yaw in radians.
    pub yaw_rad: f32,
}

/// Raw velocity setpoint command payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_velocity_setpoint_command_t {
    /// X velocity in meters per second.
    pub vx_mps: f32,
    /// Y velocity in meters per second.
    pub vy_mps: f32,
    /// Z velocity in meters per second.
    pub vz_mps: f32,
    /// Yaw rate in radians per second.
    pub yaw_rate_radps: f32,
    /// Non-zero when velocities are expressed in the body frame.
    pub body_frame: u8,
}

/// Raw vehicle core state snapshot payload.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_vehicle_core_state_t {
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

/// Raw authority lease returned by `yunlink_authority_current`.
#[repr(C)]
#[derive(Clone, Copy, Debug, Default)]
pub struct yunlink_authority_lease_t {
    /// One of the `YUNLINK_AUTHORITY_STATE_*` constants.
    pub state: u8,
    /// Session currently associated with the lease.
    pub session_id: u64,
    /// Target covered by the lease.
    pub target: yunlink_target_selector_t,
    /// One of the `YUNLINK_CONTROL_SOURCE_*` constants.
    pub source: u8,
    /// Lease time to live in milliseconds.
    pub lease_ttl_ms: u32,
    /// Absolute expiry timestamp in runtime milliseconds.
    pub expires_at_ms: u64,
    /// Peer associated with the lease.
    pub peer: yunlink_peer_t,
}
