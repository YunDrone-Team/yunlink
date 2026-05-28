use yunlink_sys as sys;

use crate::ffi_util::string_from_c_buf;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AgentType {
    GroundStation,
    Uav,
}

impl AgentType {
    pub(crate) fn to_native(self) -> u8 {
        match self {
            Self::GroundStation => sys::YUNLINK_AGENT_TYPE_GROUND_STATION,
            Self::Uav => sys::YUNLINK_AGENT_TYPE_UAV,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ControlSource {
    GroundStation,
}

impl ControlSource {
    pub(crate) fn to_native(self) -> u8 {
        match self {
            Self::GroundStation => sys::YUNLINK_CONTROL_SOURCE_GROUND_STATION,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AuthorityState {
    Controller,
    Other(u8),
}

#[derive(Debug, Clone, PartialEq)]
pub struct RuntimeConfig {
    pub udp_bind_port: u16,
    pub udp_target_port: u16,
    pub tcp_listen_port: u16,
    pub agent_type: AgentType,
    pub agent_id: u32,
}

impl RuntimeConfig {
    pub fn new(
        udp_bind_port: u16,
        udp_target_port: u16,
        tcp_listen_port: u16,
        agent_type: AgentType,
        agent_id: u32,
    ) -> Self {
        Self {
            udp_bind_port,
            udp_target_port,
            tcp_listen_port,
            agent_type,
            agent_id,
        }
    }
}

#[derive(Debug, Clone)]
pub struct PeerConnection {
    pub(crate) raw: sys::yunlink_peer_t,
    pub id: String,
}

impl PeerConnection {
    pub(crate) fn from_raw(raw: sys::yunlink_peer_t) -> Self {
        Self {
            id: string_from_c_buf(&raw.id),
            raw,
        }
    }
}

impl PartialEq for PeerConnection {
    fn eq(&self, other: &Self) -> bool {
        self.id == other.id
    }
}

impl Eq for PeerConnection {}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Session {
    pub session_id: u64,
}

impl Session {
    pub(crate) fn to_native(self) -> sys::yunlink_session_t {
        sys::yunlink_session_t {
            session_id: self.session_id,
        }
    }
}

#[derive(Debug, Clone, Copy)]
pub struct TargetSelector {
    pub(crate) raw: sys::yunlink_target_selector_t,
}

impl TargetSelector {
    pub fn entity(agent_type: AgentType, entity_id: u32) -> Self {
        Self {
            raw: sys::yunlink_target_selector_t {
                struct_size: std::mem::size_of::<sys::yunlink_target_selector_t>(),
                scope: sys::YUNLINK_TARGET_SCOPE_ENTITY,
                target_type: agent_type.to_native(),
                entity_id,
                group_id: 0,
            },
        }
    }

    pub fn broadcast(agent_type: AgentType) -> Self {
        Self {
            raw: sys::yunlink_target_selector_t {
                struct_size: std::mem::size_of::<sys::yunlink_target_selector_t>(),
                scope: 3,
                target_type: agent_type.to_native(),
                entity_id: 0,
                group_id: 0,
            },
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GotoCommand {
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub yaw_rad: f32,
}

#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VehicleCoreState {
    pub armed: bool,
    pub nav_mode: u8,
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    pub battery_percent: f32,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommandHandle {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AuthorityLease {
    pub state: AuthorityState,
    pub session_id: u64,
    pub peer: PeerConnection,
}
