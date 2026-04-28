use std::ffi::CString;
use std::fmt;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::Duration;

use thiserror::Error;
use tokio::sync::broadcast;
use yunlink_sys as sys;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum AgentType {
    GroundStation,
    Uav,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ControlSource {
    GroundStation,
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
    raw: sys::yunlink_peer_t,
    pub id: String,
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

#[derive(Debug, Clone, Copy)]
pub struct TargetSelector {
    raw: sys::yunlink_target_selector_t,
}

impl TargetSelector {
    pub fn entity(agent_type: AgentType, entity_id: u32) -> Self {
        Self {
            raw: sys::yunlink_target_selector_t {
                struct_size: std::mem::size_of::<sys::yunlink_target_selector_t>(),
                scope: sys::YUNLINK_TARGET_SCOPE_ENTITY,
                target_type: match agent_type {
                    AgentType::GroundStation => sys::YUNLINK_AGENT_TYPE_GROUND_STATION,
                    AgentType::Uav => sys::YUNLINK_AGENT_TYPE_UAV,
                },
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
                target_type: match agent_type {
                    AgentType::GroundStation => sys::YUNLINK_AGENT_TYPE_GROUND_STATION,
                    AgentType::Uav => sys::YUNLINK_AGENT_TYPE_UAV,
                },
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

#[derive(Debug, Clone, PartialEq)]
pub struct CommandResultEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub progress_percent: u8,
}

#[derive(Debug, Clone, PartialEq)]
pub struct VehicleCoreStateEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub armed: bool,
    pub battery_percent: f32,
}

#[derive(Debug, Clone, PartialEq)]
pub struct LinkEvent {
    pub peer_id: String,
    pub is_up: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ErrorEvent {
    pub code: u16,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq)]
pub enum Event {
    Link(LinkEvent),
    Error(ErrorEvent),
    CommandResult(CommandResultEvent),
    VehicleCoreState(VehicleCoreStateEvent),
}

pub const EVENT_CHANNEL_CAPACITY: usize = 64;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FfiErrorCode {
    InvalidArgument,
    SocketError,
    BindError,
    ListenError,
    ConnectError,
    Timeout,
    EncodeError,
    DecodeError,
    ChecksumMismatch,
    InvalidHeader,
    RuntimeStopped,
    ProtocolMismatch,
    Unauthorized,
    Rejected,
    Internal,
    NotFound,
    Unknown(sys::yunlink_result_t),
}

impl FfiErrorCode {
    fn from_raw(code: sys::yunlink_result_t) -> Self {
        match code {
            sys::YUNLINK_RESULT_INVALID_ARGUMENT => Self::InvalidArgument,
            sys::YUNLINK_RESULT_SOCKET_ERROR => Self::SocketError,
            sys::YUNLINK_RESULT_BIND_ERROR => Self::BindError,
            sys::YUNLINK_RESULT_LISTEN_ERROR => Self::ListenError,
            sys::YUNLINK_RESULT_CONNECT_ERROR => Self::ConnectError,
            sys::YUNLINK_RESULT_TIMEOUT => Self::Timeout,
            sys::YUNLINK_RESULT_ENCODE_ERROR => Self::EncodeError,
            sys::YUNLINK_RESULT_DECODE_ERROR => Self::DecodeError,
            sys::YUNLINK_RESULT_CHECKSUM_MISMATCH => Self::ChecksumMismatch,
            sys::YUNLINK_RESULT_INVALID_HEADER => Self::InvalidHeader,
            sys::YUNLINK_RESULT_RUNTIME_STOPPED => Self::RuntimeStopped,
            sys::YUNLINK_RESULT_PROTOCOL_MISMATCH => Self::ProtocolMismatch,
            sys::YUNLINK_RESULT_UNAUTHORIZED => Self::Unauthorized,
            sys::YUNLINK_RESULT_REJECTED => Self::Rejected,
            sys::YUNLINK_RESULT_INTERNAL => Self::Internal,
            sys::YUNLINK_RESULT_NOT_FOUND => Self::NotFound,
            other => Self::Unknown(other),
        }
    }

    fn as_name(self) -> &'static str {
        match self {
            Self::InvalidArgument => "YUNLINK_RESULT_INVALID_ARGUMENT",
            Self::SocketError => "YUNLINK_RESULT_SOCKET_ERROR",
            Self::BindError => "YUNLINK_RESULT_BIND_ERROR",
            Self::ListenError => "YUNLINK_RESULT_LISTEN_ERROR",
            Self::ConnectError => "YUNLINK_RESULT_CONNECT_ERROR",
            Self::Timeout => "YUNLINK_RESULT_TIMEOUT",
            Self::EncodeError => "YUNLINK_RESULT_ENCODE_ERROR",
            Self::DecodeError => "YUNLINK_RESULT_DECODE_ERROR",
            Self::ChecksumMismatch => "YUNLINK_RESULT_CHECKSUM_MISMATCH",
            Self::InvalidHeader => "YUNLINK_RESULT_INVALID_HEADER",
            Self::RuntimeStopped => "YUNLINK_RESULT_RUNTIME_STOPPED",
            Self::ProtocolMismatch => "YUNLINK_RESULT_PROTOCOL_MISMATCH",
            Self::Unauthorized => "YUNLINK_RESULT_UNAUTHORIZED",
            Self::Rejected => "YUNLINK_RESULT_REJECTED",
            Self::Internal => "YUNLINK_RESULT_INTERNAL",
            Self::NotFound => "YUNLINK_RESULT_NOT_FOUND",
            Self::Unknown(_) => "YUNLINK_RESULT_UNKNOWN",
        }
    }
}

impl fmt::Display for FfiErrorCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Unknown(raw) => write!(f, "{}({raw})", self.as_name()),
            _ => f.write_str(self.as_name()),
        }
    }
}

#[derive(Debug, Error)]
pub enum YunlinkError {
    #[error("{0}")]
    Ffi(FfiErrorCode),
    #[error(transparent)]
    Nul(#[from] std::ffi::NulError),
    #[error("event channel closed")]
    EventChannelClosed,
}

type Result<T> = std::result::Result<T, YunlinkError>;

#[derive(Debug, Clone, Copy)]
struct RawRuntime(*mut sys::yunlink_runtime_t);

unsafe impl Send for RawRuntime {}
unsafe impl Sync for RawRuntime {}

impl RawRuntime {
    fn ptr(self) -> *mut sys::yunlink_runtime_t {
        self.0
    }
}

pub struct Runtime {
    raw: Mutex<RawRuntime>,
    shutdown: std::sync::Arc<AtomicBool>,
    sender: broadcast::Sender<Event>,
    poll_thread: Option<JoinHandle<()>>,
}

impl Runtime {
    pub fn start(config: RuntimeConfig) -> Result<Self> {
        let mut raw_ptr = std::ptr::null_mut();
        ensure(unsafe { sys::yunlink_runtime_create(&mut raw_ptr) })?;

        let mut native_cfg = sys::yunlink_runtime_config_t::default();
        native_cfg.udp_bind_port = config.udp_bind_port;
        native_cfg.udp_target_port = config.udp_target_port;
        native_cfg.tcp_listen_port = config.tcp_listen_port;
        native_cfg.connect_timeout_ms = 5000;
        native_cfg.io_poll_interval_ms = 5;
        native_cfg.max_buffer_bytes_per_peer = 1 << 20;
        native_cfg.self_identity.agent_type = match config.agent_type {
            AgentType::GroundStation => sys::YUNLINK_AGENT_TYPE_GROUND_STATION,
            AgentType::Uav => sys::YUNLINK_AGENT_TYPE_UAV,
        };
        native_cfg.self_identity.agent_id = config.agent_id;
        native_cfg.self_identity.role = match config.agent_type {
            AgentType::GroundStation => sys::YUNLINK_ROLE_CONTROLLER,
            AgentType::Uav => sys::YUNLINK_ROLE_VEHICLE,
        };
        write_c_buffer(&mut native_cfg.shared_secret, "yunlink-secret");
        write_c_buffer(&mut native_cfg.multicast_group, "224.1.1.1");

        ensure(unsafe { sys::yunlink_runtime_start(raw_ptr, &native_cfg) })?;

        let (sender, _) = broadcast::channel(EVENT_CHANNEL_CAPACITY);
        let shutdown = std::sync::Arc::new(AtomicBool::new(false));
        let thread_shutdown = shutdown.clone();
        let thread_sender = sender.clone();
        let thread_raw = RawRuntime(raw_ptr);

        let poll_thread = thread::spawn(move || {
            while !thread_shutdown.load(Ordering::Relaxed) {
                let mut event = sys::yunlink_runtime_event_t::default();
                let result =
                    unsafe { sys::yunlink_runtime_poll_event(thread_raw.ptr(), &mut event) };
                if result == sys::YUNLINK_RESULT_OK
                    && event.type_ != sys::YUNLINK_RUNTIME_EVENT_NONE
                {
                    if let Some(parsed) = parse_event(event) {
                        let _ = thread_sender.send(parsed);
                    }
                    continue;
                }
                thread::sleep(Duration::from_millis(10));
            }
        });

        Ok(Self {
            raw: Mutex::new(RawRuntime(raw_ptr)),
            shutdown,
            sender,
            poll_thread: Some(poll_thread),
        })
    }

    pub fn subscribe(&self) -> broadcast::Receiver<Event> {
        self.sender.subscribe()
    }

    pub async fn connect(&self, ip: &str, port: u16) -> Result<PeerConnection> {
        let ip = CString::new(ip)?;
        let mut peer = sys::yunlink_peer_t::default();
        ensure(unsafe { sys::yunlink_peer_connect(self.raw_ptr(), ip.as_ptr(), port, &mut peer) })?;
        Ok(PeerConnection {
            id: string_from_c_buf(&peer.id),
            raw: peer,
        })
    }

    pub async fn open_session(&self, peer: &PeerConnection, node_name: &str) -> Result<Session> {
        let node_name = CString::new(node_name)?;
        let mut session = sys::yunlink_session_t::default();
        ensure(unsafe {
            sys::yunlink_session_open(self.raw_ptr(), &peer.raw, node_name.as_ptr(), &mut session)
        })?;
        Ok(Session {
            session_id: session.session_id,
        })
    }

    pub async fn request_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        source: ControlSource,
        lease_ttl_ms: u32,
        allow_preempt: bool,
    ) -> Result<()> {
        let session = sys::yunlink_session_t {
            session_id: session.session_id,
        };
        let source = match source {
            ControlSource::GroundStation => sys::YUNLINK_CONTROL_SOURCE_GROUND_STATION,
        };
        ensure(unsafe {
            sys::yunlink_authority_request(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                source,
                lease_ttl_ms,
                if allow_preempt { 1 } else { 0 },
            )
        })
    }

    pub async fn release_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<()> {
        let session = sys::yunlink_session_t {
            session_id: session.session_id,
        };
        ensure(unsafe {
            sys::yunlink_authority_release(self.raw_ptr(), &peer.raw, &session, &target.raw)
        })
    }

    pub async fn renew_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        source: ControlSource,
        lease_ttl_ms: u32,
    ) -> Result<()> {
        let session = sys::yunlink_session_t {
            session_id: session.session_id,
        };
        let source = match source {
            ControlSource::GroundStation => sys::YUNLINK_CONTROL_SOURCE_GROUND_STATION,
        };
        ensure(unsafe {
            sys::yunlink_authority_renew(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                source,
                lease_ttl_ms,
            )
        })
    }

    pub fn current_authority(&self) -> Result<Option<AuthorityLease>> {
        let mut lease = sys::yunlink_authority_lease_t::default();
        let result = unsafe { sys::yunlink_authority_current(self.raw_ptr(), &mut lease) };
        if result == sys::YUNLINK_RESULT_NOT_FOUND {
            return Ok(None);
        }
        ensure(result)?;
        Ok(Some(AuthorityLease {
            state: if lease.state == sys::YUNLINK_AUTHORITY_STATE_CONTROLLER {
                AuthorityState::Controller
            } else {
                AuthorityState::Other(lease.state)
            },
            session_id: lease.session_id,
            peer: PeerConnection {
                id: string_from_c_buf(&lease.peer.id),
                raw: lease.peer,
            },
        }))
    }

    pub async fn publish_goto(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        command: &GotoCommand,
    ) -> Result<CommandHandle> {
        let session = sys::yunlink_session_t {
            session_id: session.session_id,
        };
        let payload = sys::yunlink_goto_command_t {
            x_m: command.x_m,
            y_m: command.y_m,
            z_m: command.z_m,
            yaw_rad: command.yaw_rad,
        };
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_command_publish_goto(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &payload,
                &mut handle,
            )
        })?;
        Ok(CommandHandle {
            session_id: handle.session_id,
            message_id: handle.message_id,
            correlation_id: handle.correlation_id,
        })
    }

    pub async fn publish_vehicle_core_state(
        &self,
        peer: &PeerConnection,
        target: &TargetSelector,
        state: VehicleCoreState,
        session_id: u64,
    ) -> Result<()> {
        let payload = sys::yunlink_vehicle_core_state_t {
            armed: if state.armed { 1 } else { 0 },
            nav_mode: state.nav_mode,
            x_m: state.x_m,
            y_m: state.y_m,
            z_m: state.z_m,
            vx_mps: state.vx_mps,
            vy_mps: state.vy_mps,
            vz_mps: state.vz_mps,
            battery_percent: state.battery_percent,
        };
        ensure(unsafe {
            sys::yunlink_publish_vehicle_core_state(
                self.raw_ptr(),
                &peer.raw,
                &target.raw,
                &payload,
                session_id,
            )
        })
    }

    fn raw_ptr(&self) -> *mut sys::yunlink_runtime_t {
        self.raw_lock().0
    }

    fn raw_lock(&self) -> MutexGuard<'_, RawRuntime> {
        self.raw.lock().expect("raw runtime mutex poisoned")
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        self.shutdown.store(true, Ordering::Relaxed);
        if let Some(handle) = self.poll_thread.take() {
            let _ = handle.join();
        }
        let raw = self.raw.lock().expect("raw runtime mutex poisoned").0;
        let _ = unsafe { sys::yunlink_runtime_stop(raw) };
        unsafe { sys::yunlink_runtime_destroy(raw) };
    }
}

fn ensure(code: sys::yunlink_result_t) -> Result<()> {
    if code == sys::YUNLINK_RESULT_OK {
        return Ok(());
    }
    Err(YunlinkError::Ffi(FfiErrorCode::from_raw(code)))
}

fn write_c_buffer<const N: usize>(dst: &mut [std::ffi::c_char; N], value: &str) {
    for slot in dst.iter_mut() {
        *slot = 0;
    }
    for (index, byte) in value
        .as_bytes()
        .iter()
        .copied()
        .enumerate()
        .take(N.saturating_sub(1))
    {
        dst[index] = byte as std::ffi::c_char;
    }
}

fn string_from_c_buf<const N: usize>(buf: &[std::ffi::c_char; N]) -> String {
    let len = buf.iter().position(|value| *value == 0).unwrap_or(N);
    let bytes: Vec<u8> = buf[..len].iter().map(|value| *value as u8).collect();
    String::from_utf8_lossy(&bytes).into_owned()
}

fn parse_event(event: sys::yunlink_runtime_event_t) -> Option<Event> {
    match event.type_ {
        sys::YUNLINK_RUNTIME_EVENT_LINK => {
            let data = unsafe { event.data.link };
            Some(Event::Link(LinkEvent {
                peer_id: string_from_c_buf(&data.peer_id),
                is_up: data.is_up != 0,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_ERROR => {
            let data = unsafe { event.data.error };
            Some(Event::Error(ErrorEvent {
                code: data.code,
                message: string_from_c_buf(&data.message),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_COMMAND_RESULT => {
            let data = unsafe { event.data.command_result };
            Some(Event::CommandResult(CommandResultEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                progress_percent: data.progress_percent,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE => {
            let data = unsafe { event.data.vehicle_core_state };
            Some(Event::VehicleCoreState(VehicleCoreStateEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                armed: data.armed != 0,
                battery_percent: data.battery_percent,
            }))
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::FfiErrorCode;
    use yunlink_sys as sys;

    #[test]
    fn ffi_error_code_mapping_is_complete_for_stable_result_set() {
        let stable_pairs = [
            (sys::YUNLINK_RESULT_INVALID_ARGUMENT, FfiErrorCode::InvalidArgument),
            (sys::YUNLINK_RESULT_SOCKET_ERROR, FfiErrorCode::SocketError),
            (sys::YUNLINK_RESULT_BIND_ERROR, FfiErrorCode::BindError),
            (sys::YUNLINK_RESULT_LISTEN_ERROR, FfiErrorCode::ListenError),
            (sys::YUNLINK_RESULT_CONNECT_ERROR, FfiErrorCode::ConnectError),
            (sys::YUNLINK_RESULT_TIMEOUT, FfiErrorCode::Timeout),
            (sys::YUNLINK_RESULT_ENCODE_ERROR, FfiErrorCode::EncodeError),
            (sys::YUNLINK_RESULT_DECODE_ERROR, FfiErrorCode::DecodeError),
            (sys::YUNLINK_RESULT_CHECKSUM_MISMATCH, FfiErrorCode::ChecksumMismatch),
            (sys::YUNLINK_RESULT_INVALID_HEADER, FfiErrorCode::InvalidHeader),
            (sys::YUNLINK_RESULT_RUNTIME_STOPPED, FfiErrorCode::RuntimeStopped),
            (sys::YUNLINK_RESULT_PROTOCOL_MISMATCH, FfiErrorCode::ProtocolMismatch),
            (sys::YUNLINK_RESULT_UNAUTHORIZED, FfiErrorCode::Unauthorized),
            (sys::YUNLINK_RESULT_REJECTED, FfiErrorCode::Rejected),
            (sys::YUNLINK_RESULT_INTERNAL, FfiErrorCode::Internal),
            (sys::YUNLINK_RESULT_NOT_FOUND, FfiErrorCode::NotFound),
        ];

        for (raw, expected) in stable_pairs {
            assert_eq!(FfiErrorCode::from_raw(raw), expected, "raw={raw}");
            assert!(
                FfiErrorCode::from_raw(raw)
                    .to_string()
                    .starts_with("YUNLINK_RESULT_"),
                "raw={raw}"
            );
        }

        assert_eq!(FfiErrorCode::from_raw(999), FfiErrorCode::Unknown(999));
        assert_eq!(FfiErrorCode::Unknown(999).to_string(), "YUNLINK_RESULT_UNKNOWN(999)");
    }
}
