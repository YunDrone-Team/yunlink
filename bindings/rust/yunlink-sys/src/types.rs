use std::ffi::c_char;

pub type yunlink_runtime_t = core::ffi::c_void;

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
