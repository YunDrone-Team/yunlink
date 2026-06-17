use yunlink_sys as sys;

use crate::ffi_util::string_from_c_buf;

mod commands;

pub use commands::{
    CommandHandle, GotoCommand, LandCommand, ReturnCommand, TakeoffCommand, TargetSelector,
    VehicleCoreState, VelocitySetpointCommand,
};

/// Endpoint agent type used by the safe Rust SDK.
///
/// Unknown values are preserved so newer C ABI constants can flow through older
/// Rust callers without panicking.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AgentType {
    /// Ground station / controller endpoint.
    GroundStation,
    /// Unmanned aerial vehicle endpoint.
    Uav,
    /// Unmanned ground vehicle endpoint.
    Ugv,
    /// Swarm-level controller endpoint.
    SwarmController,
    /// Unknown future or vendor-specific value.
    Unknown(u8),
}

impl AgentType {
    pub(crate) fn to_native(self) -> u8 {
        match self {
            Self::GroundStation => sys::YUNLINK_AGENT_TYPE_GROUND_STATION,
            Self::Uav => sys::YUNLINK_AGENT_TYPE_UAV,
            Self::Ugv => sys::YUNLINK_AGENT_TYPE_UGV,
            Self::SwarmController => sys::YUNLINK_AGENT_TYPE_SWARM_CONTROLLER,
            Self::Unknown(value) => value,
        }
    }

    pub(crate) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_AGENT_TYPE_GROUND_STATION => Self::GroundStation,
            sys::YUNLINK_AGENT_TYPE_UAV => Self::Uav,
            sys::YUNLINK_AGENT_TYPE_UGV => Self::Ugv,
            sys::YUNLINK_AGENT_TYPE_SWARM_CONTROLLER => Self::SwarmController,
            other => Self::Unknown(other),
        }
    }
}

/// Source of control authority requests.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ControlSource {
    /// Ground-station originated control.
    GroundStation,
}

impl ControlSource {
    pub(crate) fn to_native(self) -> u8 {
        match self {
            Self::GroundStation => sys::YUNLINK_CONTROL_SOURCE_GROUND_STATION,
        }
    }
}

/// Authority state mirrored from the C ABI.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AuthorityState {
    /// Observer only; no command authority.
    Observer,
    /// Authority request is pending.
    PendingGrant,
    /// This session currently controls the target.
    Controller,
    /// Authority is being preempted.
    Preempting,
    /// Authority was revoked.
    Revoked,
    /// Authority was released.
    Released,
    /// Authority request was rejected.
    Rejected,
    /// Unknown future or vendor-specific value.
    Other(u8),
}

impl AuthorityState {
    pub(crate) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_AUTHORITY_STATE_OBSERVER => Self::Observer,
            sys::YUNLINK_AUTHORITY_STATE_PENDING_GRANT => Self::PendingGrant,
            sys::YUNLINK_AUTHORITY_STATE_CONTROLLER => Self::Controller,
            sys::YUNLINK_AUTHORITY_STATE_PREEMPTING => Self::Preempting,
            sys::YUNLINK_AUTHORITY_STATE_REVOKED => Self::Revoked,
            sys::YUNLINK_AUTHORITY_STATE_RELEASED => Self::Released,
            sys::YUNLINK_AUTHORITY_STATE_REJECTED => Self::Rejected,
            other => Self::Other(other),
        }
    }
}

/// Session state mirrored from `yunlink_session_info_t`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum SessionState {
    /// Session was discovered but has not started a handshake.
    Discovered,
    /// Session is exchanging handshake data.
    Handshaking,
    /// Session passed authentication.
    Authenticated,
    /// Session completed capability negotiation.
    Negotiated,
    /// Session is active and usable.
    Active,
    /// Session is draining before close.
    Draining,
    /// Session closed normally.
    Closed,
    /// Session was lost unexpectedly.
    Lost,
    /// Session handle is invalid.
    Invalid,
    /// Unknown future or vendor-specific value.
    Other(u8),
}

impl SessionState {
    pub(crate) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_SESSION_STATE_DISCOVERED => Self::Discovered,
            sys::YUNLINK_SESSION_STATE_HANDSHAKING => Self::Handshaking,
            sys::YUNLINK_SESSION_STATE_AUTHENTICATED => Self::Authenticated,
            sys::YUNLINK_SESSION_STATE_NEGOTIATED => Self::Negotiated,
            sys::YUNLINK_SESSION_STATE_ACTIVE => Self::Active,
            sys::YUNLINK_SESSION_STATE_DRAINING => Self::Draining,
            sys::YUNLINK_SESSION_STATE_CLOSED => Self::Closed,
            sys::YUNLINK_SESSION_STATE_LOST => Self::Lost,
            sys::YUNLINK_SESSION_STATE_INVALID => Self::Invalid,
            other => Self::Other(other),
        }
    }
}

/// Safe runtime configuration.
///
/// `Runtime::start` converts this into `yunlink_runtime_config_t`, including
/// fixed C string buffers for `shared_secret` and `multicast_group`.
#[derive(Debug, Clone, PartialEq)]
pub struct RuntimeConfig {
    /// UDP bind port.
    pub udp_bind_port: u16,
    /// UDP target port.
    pub udp_target_port: u16,
    /// TCP listen port.
    pub tcp_listen_port: u16,
    /// Local endpoint agent type.
    pub agent_type: AgentType,
    /// Local endpoint agent id.
    pub agent_id: u32,
    /// Shared secret copied into the C ABI fixed-size buffer.
    pub shared_secret: String,
    /// Multicast group copied into the C ABI fixed-size buffer.
    pub multicast_group: String,
}

impl RuntimeConfig {
    /// Create a runtime configuration with stable SDK defaults.
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
            shared_secret: "yunlink-secret".to_string(),
            multicast_group: "224.1.1.1".to_string(),
        }
    }

    /// Override the shared secret used by session authentication.
    pub fn with_shared_secret(mut self, shared_secret: impl Into<String>) -> Self {
        self.shared_secret = shared_secret.into();
        self
    }

    /// Override the multicast group used by the runtime.
    pub fn with_multicast_group(mut self, multicast_group: impl Into<String>) -> Self {
        self.multicast_group = multicast_group.into();
        self
    }
}

/// Safe peer connection handle.
///
/// Internally this wraps the C ABI `yunlink_peer_t`, but callers normally use
/// the rendered `id` string.
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

/// Safe session handle.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Session {
    /// Protocol session id.
    pub session_id: u64,
}

impl Session {
    pub(crate) fn to_native(self) -> sys::yunlink_session_t {
        sys::yunlink_session_t {
            session_id: self.session_id,
        }
    }
}

/// Safe session description returned by `Runtime::describe_session`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SessionInfo {
    /// Described session id.
    pub session_id: u64,
    /// Current session state.
    pub state: SessionState,
    /// Remote endpoint agent type.
    pub remote_agent_type: AgentType,
    /// Remote endpoint agent id.
    pub remote_agent_id: u32,
    /// Remote peer handle.
    pub peer: PeerConnection,
    /// Negotiated capability flags.
    pub capability_flags: u32,
    /// Remote node name.
    pub node_name: String,
}

/// Safe authority lease snapshot.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct AuthorityLease {
    /// Current authority state.
    pub state: AuthorityState,
    /// Session currently associated with the lease.
    pub session_id: u64,
    /// Peer associated with the lease.
    pub peer: PeerConnection,
}
