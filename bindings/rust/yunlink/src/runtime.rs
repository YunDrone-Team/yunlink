use std::ffi::CString;
use std::sync::atomic::{AtomicBool, Ordering};
use std::sync::{Mutex, MutexGuard};
use std::thread::{self, JoinHandle};
use std::time::Duration;

use tokio::sync::broadcast;
use yunlink_sys as sys;

use crate::error::{ensure, Result};
use crate::events::{parse_event, Event, EVENT_CHANNEL_CAPACITY};
use crate::ffi_util::write_c_buffer;
use crate::types::{
    AgentType, AuthorityLease, AuthorityState, CommandHandle, ControlSource, GotoCommand,
    PeerConnection, RuntimeConfig, Session, TargetSelector, VehicleCoreState,
};

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
        native_cfg.self_identity.agent_type = config.agent_type.to_native();
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
        Ok(PeerConnection::from_raw(peer))
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
            peer: PeerConnection::from_raw(lease.peer),
        }))
    }

    pub async fn publish_goto(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        command: &GotoCommand,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
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
