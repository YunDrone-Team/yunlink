use std::ffi::CString;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::Duration;

use tokio::sync::broadcast;
use yunlink_sys as sys;

use crate::configuration::{
    register_callbacks, ConfigurationCallbackContext, ConfigurationResponse,
};
use crate::error::{ensure, Result};
use crate::events::{parse_event, Event, EVENT_CHANNEL_CAPACITY};
use crate::ffi_util::write_c_buffer;
use crate::types::{
    AgentType, AuthorityLease, AuthorityState, CommandHandle, ControlSource, PeerConnection,
    RuntimeConfig, Session, SessionInfo, SessionState, TargetSelector,
};

mod commands;
mod configuration;

/// Raw C ABI runtime pointer.
///
#[derive(Debug, Clone, Copy)]
struct RawRuntime(*mut sys::yunlink_runtime_t);

// The C++ runtime is internally synchronized for the operations exposed by this
// wrapper. We still keep the pointer behind a Mutex in `Runtime` so all safe
// method calls retrieve it through one consistent path.
unsafe impl Send for RawRuntime {}
unsafe impl Sync for RawRuntime {}

impl RawRuntime {
    fn ptr(self) -> *mut sys::yunlink_runtime_t {
        self.0
    }
}

/// Safe Rust runtime facade over the bindings-oriented C ABI.
///
/// The object owns exactly one opaque `yunlink_runtime_t*`. Public methods use
/// Rust domain types, convert them into `yunlink_*_t` C structs, call the raw
/// `yunlink-sys` symbols, and map result codes back into `YunlinkError`.
pub struct Runtime {
    /// Opaque C ABI runtime handle.
    raw: Mutex<RawRuntime>,
    /// Signals the event polling thread to stop.
    shutdown: std::sync::Arc<AtomicBool>,
    /// Broadcast channel used by Rust subscribers.
    sender: broadcast::Sender<Event>,
    configuration_sender: broadcast::Sender<ConfigurationResponse>,
    configuration_callback_context: Box<ConfigurationCallbackContext>,
    configuration_tokens: Vec<usize>,
    /// Background thread that polls `yunlink_runtime_poll_event`.
    poll_thread: Option<JoinHandle<()>>,
}

impl Runtime {
    /// Create, configure, and start a YunLink runtime through C ABI.
    pub fn start(config: RuntimeConfig) -> Result<Self> {
        let mut raw_ptr = std::ptr::null_mut();
        ensure(unsafe { sys::yunlink_runtime_create(&mut raw_ptr) })?;

        // Build the C ABI config struct explicitly. The `Default`
        // implementation supplies `struct_size` and zeroed fixed string
        // buffers; the helper below then fills those buffers safely.
        let mut native_cfg = sys::yunlink_runtime_config_t {
            udp_bind_port: config.udp_bind_port,
            udp_target_port: config.udp_target_port,
            tcp_listen_port: config.tcp_listen_port,
            connect_timeout_ms: 5000,
            io_poll_interval_ms: 5,
            max_buffer_bytes_per_peer: 1 << 20,
            self_identity: sys::yunlink_identity_t {
                agent_type: config.agent_type.to_native(),
                agent_id: config.agent_id,
                role: match config.agent_type {
                    AgentType::GroundStation => sys::YUNLINK_ROLE_CONTROLLER,
                    AgentType::Uav | AgentType::Ugv => sys::YUNLINK_ROLE_VEHICLE,
                    AgentType::SwarmController => sys::YUNLINK_ROLE_CONTROLLER,
                    AgentType::Unknown(_) => sys::YUNLINK_ROLE_OBSERVER,
                },
            },
            security_tags_enabled: 1,
            ..Default::default()
        };
        write_c_buffer(&mut native_cfg.shared_secret, &config.shared_secret);
        write_c_buffer(&mut native_cfg.multicast_group, &config.multicast_group);

        ensure(unsafe { sys::yunlink_runtime_start(raw_ptr, &native_cfg) })?;

        let (sender, _) = broadcast::channel(EVENT_CHANNEL_CAPACITY);
        let (configuration_sender, _) = broadcast::channel(EVENT_CHANNEL_CAPACITY);
        let (configuration_callback_context, configuration_tokens) =
            match register_callbacks(raw_ptr, configuration_sender.clone()) {
                Ok(registration) => registration,
                Err(error) => {
                    let _ = unsafe { sys::yunlink_runtime_stop(raw_ptr) };
                    unsafe { sys::yunlink_runtime_destroy(raw_ptr) };
                    return Err(error);
                }
            };
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
            configuration_sender,
            configuration_callback_context,
            configuration_tokens,
            poll_thread: Some(poll_thread),
        })
    }

    /// Subscribe to parsed runtime events.
    pub fn subscribe(&self) -> broadcast::Receiver<Event> {
        self.sender.subscribe()
    }

    /// Subscribe to owned configuration service responses.
    pub fn subscribe_configuration(&self) -> broadcast::Receiver<ConfigurationResponse> {
        self.configuration_sender.subscribe()
    }

    /// Connect to a remote peer through the C ABI TCP client helper.
    pub async fn connect(&self, ip: &str, port: u16) -> Result<PeerConnection> {
        let ip = CString::new(ip)?;
        let mut peer = sys::yunlink_peer_t::default();
        ensure(unsafe { sys::yunlink_peer_connect(self.raw_ptr(), ip.as_ptr(), port, &mut peer) })?;
        Ok(PeerConnection::from_raw(peer))
    }

    /// Open an active session with a connected peer.
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

    /// Describe an existing session, if the runtime still knows it.
    pub fn describe_session(&self, session: &Session) -> Result<Option<SessionInfo>> {
        let native_session = session.to_native();
        let mut info = sys::yunlink_session_info_t::default();
        let result =
            unsafe { sys::yunlink_session_describe(self.raw_ptr(), &native_session, &mut info) };
        if result == sys::YUNLINK_RESULT_NOT_FOUND {
            return Ok(None);
        }
        ensure(result)?;
        Ok(Some(SessionInfo {
            session_id: info.session_id,
            state: SessionState::from_native(info.state),
            remote_agent_type: AgentType::from_native(info.remote_identity.agent_type),
            remote_agent_id: info.remote_identity.agent_id,
            peer: PeerConnection::from_raw(info.peer),
            capability_flags: info.capability_flags,
            node_name: crate::ffi_util::string_from_c_buf(&info.node_name),
        }))
    }

    /// Request authority for a target.
    pub async fn request_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        source: ControlSource,
        lease_ttl_ms: u32,
        allow_preempt: bool,
    ) -> Result<()> {
        let session = session.to_native();
        ensure(unsafe {
            sys::yunlink_authority_request(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                source.to_native(),
                lease_ttl_ms,
                if allow_preempt { 1 } else { 0 },
            )
        })
    }

    /// Release authority for a target.
    pub async fn release_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<()> {
        let session = session.to_native();
        ensure(unsafe {
            sys::yunlink_authority_release(self.raw_ptr(), &peer.raw, &session, &target.raw)
        })
    }

    /// Renew authority for a target.
    pub async fn renew_authority(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        source: ControlSource,
        lease_ttl_ms: u32,
    ) -> Result<()> {
        let session = session.to_native();
        ensure(unsafe {
            sys::yunlink_authority_renew(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                source.to_native(),
                lease_ttl_ms,
            )
        })
    }

    /// Return the current authority lease observed by this runtime.
    pub fn current_authority(&self) -> Result<Option<AuthorityLease>> {
        let mut lease = sys::yunlink_authority_lease_t::default();
        let result = unsafe { sys::yunlink_authority_current(self.raw_ptr(), &mut lease) };
        if result == sys::YUNLINK_RESULT_NOT_FOUND {
            return Ok(None);
        }
        ensure(result)?;
        Ok(Some(AuthorityLease {
            state: AuthorityState::from_native(lease.state),
            session_id: lease.session_id,
            peer: PeerConnection::from_raw(lease.peer),
        }))
    }

    /// Return the current opaque runtime pointer.
    pub(crate) fn raw_ptr(&self) -> *mut sys::yunlink_runtime_t {
        self.raw_lock().0
    }

    /// Lock the raw runtime handle.
    fn raw_lock(&self) -> MutexGuard<'_, RawRuntime> {
        self.raw.lock().expect("raw runtime mutex poisoned")
    }

    /// Convert a raw C ABI command handle into the public safe handle.
    fn command_handle_from_native(handle: sys::yunlink_command_handle_t) -> CommandHandle {
        CommandHandle {
            session_id: handle.session_id,
            message_id: handle.message_id,
            correlation_id: handle.correlation_id,
        }
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        // Stop the polling thread before destroying the opaque runtime. This
        // avoids polling a pointer after `yunlink_runtime_destroy`.
        self.shutdown.store(true, Ordering::Relaxed);
        if let Some(handle) = self.poll_thread.take() {
            let _ = handle.join();
        }
        let raw = self.raw.lock().expect("raw runtime mutex poisoned").0;
        for token in self.configuration_tokens.drain(..) {
            let _ = unsafe { sys::yunlink_configuration_unsubscribe(raw, token) };
        }
        let _ = unsafe { sys::yunlink_runtime_stop(raw) };
        unsafe { sys::yunlink_runtime_destroy(raw) };
        let _ = &self.configuration_callback_context;
    }
}
