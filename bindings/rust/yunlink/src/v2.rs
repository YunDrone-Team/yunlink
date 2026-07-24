//! Owned Rust facade for YunLink Wire v2.

use std::{ffi::c_void, slice, sync::Mutex};

use tokio::sync::broadcast;
use yunlink_sys::v2 as sys;

const EVENT_CAPACITY: usize = 512;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Error {
    pub code: u16,
}

impl std::fmt::Display for Error {
    fn fmt(&self, formatter: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(formatter, "YUNLINK_V2_ERROR({})", self.code)
    }
}

impl std::error::Error for Error {}

pub type Result<T> = std::result::Result<T, Error>;

fn ensure(code: u16) -> Result<()> {
    (code == 0).then_some(()).ok_or(Error { code })
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Profile {
    pub profile_id: String,
    pub major: u16,
    pub minor: u16,
    pub schema_digest: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TypeRef {
    pub profile_id: String,
    pub major: u16,
    pub minor: u16,
    pub type_name: String,
}

impl TypeRef {
    pub fn new(profile_id: impl Into<String>, major: u16, type_name: impl Into<String>) -> Self {
        Self {
            profile_id: profile_id.into(),
            major,
            minor: 0,
            type_name: type_name.into(),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Family {
    Session = 1,
    Authority = 2,
    EntityDirectory = 3,
    Stream = 4,
    Action = 5,
    Rpc = 6,
    Configuration = 7,
    Log = 8,
    Bulk = 9,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Qos {
    ReliableOrdered = 1,
    ReliableLatest = 2,
    BestEffort = 3,
    Bulk = 4,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Target {
    Endpoint(Vec<String>),
    Entity(Vec<String>),
    Group(Vec<String>),
    Broadcast,
}

impl Target {
    fn scope(&self) -> u8 {
        match self {
            Self::Endpoint(_) => 1,
            Self::Entity(_) => 2,
            Self::Group(_) => 3,
            Self::Broadcast => 4,
        }
    }

    fn uids(&self) -> &[String] {
        match self {
            Self::Endpoint(values) | Self::Entity(values) | Self::Group(values) => values,
            Self::Broadcast => &[],
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RuntimeConfig {
    pub endpoint_uid: String,
    pub display_name: String,
    pub shared_secret: String,
    pub tcp_listen_port: u16,
    pub profiles: Vec<Profile>,
    pub required_profiles: Vec<Profile>,
}

impl RuntimeConfig {
    pub fn new(endpoint_uid: impl Into<String>, tcp_listen_port: u16) -> Self {
        Self {
            endpoint_uid: endpoint_uid.into(),
            display_name: "yunlink-endpoint".into(),
            shared_secret: "yunlink-default-secret".into(),
            tcp_listen_port,
            profiles: Vec::new(),
            required_profiles: Vec::new(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Peer {
    pub id: String,
    pub ip: String,
    pub port: u16,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MessageHandle {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Message {
    pub peer_id: String,
    pub session_id: u64,
    pub operation: u8,
    pub qos: u8,
    pub message_id: u64,
    pub correlation_id: u64,
    pub created_at_ms: u64,
    pub ttl_ms: u32,
    pub source_endpoint_uid: String,
    pub source_entity_uid: String,
    pub target: Target,
    pub type_ref: TypeRef,
    pub payload: Vec<u8>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum Event {
    Stream(Message),
    Action(Message),
    Rpc(Message),
    Authority(Message),
    EntityDirectory(Message),
    Configuration(Message),
    Log(Message),
    Bulk(Message),
    Session {
        peer_id: String,
        session_id: u64,
        state: u8,
        authenticated: bool,
    },
    Link { peer_id: String, up: bool },
    Error { peer_id: String, code: u16, message: String },
}

struct CallbackContext {
    sender: broadcast::Sender<Event>,
}

unsafe fn copy_string(value: sys::yunlink_v2_string_view_t) -> String {
    if value.data.is_null() || value.len == 0 {
        return String::new();
    }
    String::from_utf8_lossy(slice::from_raw_parts(value.data.cast::<u8>(), value.len)).into_owned()
}

unsafe fn copy_bytes(value: sys::yunlink_v2_bytes_view_t) -> Vec<u8> {
    if value.data.is_null() || value.len == 0 {
        return Vec::new();
    }
    slice::from_raw_parts(value.data, value.len).to_vec()
}

unsafe fn copy_message(event: &sys::yunlink_v2_event_t) -> Message {
    let target_uids = if event.target.uids.is_null() || event.target.uid_count == 0 {
        Vec::new()
    } else {
        slice::from_raw_parts(event.target.uids, event.target.uid_count)
            .iter()
            .map(|value| copy_string(*value))
            .collect()
    };
    let target = match event.target.scope {
        1 => Target::Endpoint(target_uids),
        2 => Target::Entity(target_uids),
        3 => Target::Group(target_uids),
        _ => Target::Broadcast,
    };
    Message {
        peer_id: copy_string(event.peer_id),
        session_id: event.session_id,
        operation: event.operation,
        qos: event.qos_class,
        message_id: event.message_id,
        correlation_id: event.correlation_id,
        created_at_ms: event.created_at_ms,
        ttl_ms: event.ttl_ms,
        source_endpoint_uid: copy_string(event.source_endpoint_uid),
        source_entity_uid: copy_string(event.source_entity_uid),
        target,
        type_ref: TypeRef {
            profile_id: copy_string(event.type_ref.profile_id),
            major: event.type_ref.major,
            minor: event.type_ref.minor,
            type_name: copy_string(event.type_ref.type_name),
        },
        payload: copy_bytes(event.payload),
    }
}

unsafe extern "C" fn event_callback(
    event: *const sys::yunlink_v2_event_t,
    user_data: *mut c_void,
) {
    if event.is_null() || user_data.is_null() {
        return;
    }
    let event = &*event;
    let context = &*(user_data.cast::<CallbackContext>());
    let parsed = match event.kind {
        1 => {
            let message = copy_message(event);
            match event.family {
                2 => Event::Authority(message),
                3 => Event::EntityDirectory(message),
                4 => Event::Stream(message),
                5 => Event::Action(message),
                6 => Event::Rpc(message),
                7 => Event::Configuration(message),
                8 => Event::Log(message),
                9 => Event::Bulk(message),
                _ => return,
            }
        }
        2 => Event::Link {
            peer_id: copy_string(event.peer_id),
            up: event.link_up != 0,
        },
        3 => Event::Session {
            peer_id: copy_string(event.peer_id),
            session_id: event.session_id,
            state: event.session_state,
            authenticated: event.session_authenticated != 0,
        },
        4 => Event::Error {
            peer_id: copy_string(event.peer_id),
            code: event.error_code,
            message: copy_string(event.message),
        },
        _ => return,
    };
    let _ = context.sender.send(parsed);
}

fn string_view(value: &str) -> sys::yunlink_v2_string_view_t {
    sys::yunlink_v2_string_view_t {
        data: value.as_ptr().cast(),
        len: value.len(),
    }
}

fn profile_view(value: &Profile) -> sys::yunlink_v2_profile_view_t {
    sys::yunlink_v2_profile_view_t {
        profile_id: string_view(&value.profile_id),
        major: value.major,
        minor: value.minor,
        schema_digest: string_view(&value.schema_digest),
    }
}

fn c_buffer(value: &[std::ffi::c_char]) -> String {
    let len = value.iter().position(|byte| *byte == 0).unwrap_or(value.len());
    String::from_utf8_lossy(unsafe {
        slice::from_raw_parts(value.as_ptr().cast::<u8>(), len)
    })
    .into_owned()
}

struct Raw(*mut sys::yunlink_v2_runtime_t);
unsafe impl Send for Raw {}
unsafe impl Sync for Raw {}

pub struct Runtime {
    raw: Mutex<Raw>,
    sender: broadcast::Sender<Event>,
    callback_context: Box<CallbackContext>,
    callback_token: u64,
}

impl Runtime {
    pub fn start(config: RuntimeConfig) -> Result<Self> {
        if unsafe { sys::yunlink_v2_abi_version() } != 2 {
            return Err(Error { code: 5 });
        }
        let raw = unsafe { sys::yunlink_v2_runtime_create() };
        if raw.is_null() {
            return Err(Error { code: 13 });
        }
        let profiles = config.profiles.iter().map(profile_view).collect::<Vec<_>>();
        let required = config
            .required_profiles
            .iter()
            .map(profile_view)
            .collect::<Vec<_>>();
        let native = sys::yunlink_v2_runtime_config_t {
            struct_size: std::mem::size_of::<sys::yunlink_v2_runtime_config_t>(),
            endpoint_uid: string_view(&config.endpoint_uid),
            display_name: string_view(&config.display_name),
            shared_secret: string_view(&config.shared_secret),
            tcp_listen_port: config.tcp_listen_port,
            profiles: profiles.as_ptr(),
            profile_count: profiles.len(),
            required_profiles: required.as_ptr(),
            required_profile_count: required.len(),
        };
        if let Err(error) = ensure(unsafe { sys::yunlink_v2_runtime_start(raw, &native) }) {
            unsafe { sys::yunlink_v2_runtime_destroy(raw) };
            return Err(error);
        }
        let (sender, _) = broadcast::channel(EVENT_CAPACITY);
        let mut callback_context = Box::new(CallbackContext {
            sender: sender.clone(),
        });
        let callback_token = unsafe {
            sys::yunlink_v2_runtime_subscribe(
                raw,
                Some(event_callback),
                (&mut *callback_context as *mut CallbackContext).cast(),
            )
        };
        if callback_token == 0 {
            unsafe {
                sys::yunlink_v2_runtime_stop(raw);
                sys::yunlink_v2_runtime_destroy(raw);
            }
            return Err(Error { code: 13 });
        }
        Ok(Self {
            raw: Mutex::new(Raw(raw)),
            sender,
            callback_context,
            callback_token,
        })
    }

    fn raw(&self) -> *mut sys::yunlink_v2_runtime_t {
        self.raw.lock().expect("v2 runtime mutex poisoned").0
    }

    pub fn subscribe(&self) -> broadcast::Receiver<Event> {
        self.sender.subscribe()
    }

    pub async fn connect(&self, ip: &str, port: u16) -> Result<Peer> {
        let mut peer = sys::yunlink_v2_peer_t::default();
        ensure(unsafe {
            sys::yunlink_v2_runtime_connect(self.raw(), string_view(ip), port, &mut peer)
        })?;
        Ok(Peer {
            id: c_buffer(&peer.id),
            ip: c_buffer(&peer.ip),
            port: peer.port,
        })
    }

    pub async fn open_session(&self, peer: &Peer) -> Result<u64> {
        let session_id = unsafe {
            sys::yunlink_v2_runtime_open_session(self.raw(), string_view(&peer.id))
        };
        (session_id != 0)
            .then_some(session_id)
            .ok_or(Error { code: 8 })
    }

    #[allow(clippy::too_many_arguments)]
    pub fn publish(
        &self,
        peer: &Peer,
        session_id: u64,
        family: Family,
        operation: u8,
        target: &Target,
        type_ref: &TypeRef,
        payload: &[u8],
        correlation_id: u64,
        ttl_ms: u32,
        qos: Qos,
        source_entity_uid: &str,
    ) -> Result<MessageHandle> {
        let uid_views = target.uids().iter().map(|uid| string_view(uid)).collect::<Vec<_>>();
        let native_target = sys::yunlink_v2_target_view_t {
            scope: target.scope(),
            uids: uid_views.as_ptr(),
            uid_count: uid_views.len(),
        };
        let native_type = sys::yunlink_v2_type_ref_view_t {
            profile_id: string_view(&type_ref.profile_id),
            major: type_ref.major,
            minor: type_ref.minor,
            type_name: string_view(&type_ref.type_name),
        };
        let native_payload = sys::yunlink_v2_bytes_view_t {
            data: payload.as_ptr(),
            len: payload.len(),
        };
        let mut handle = sys::yunlink_v2_message_handle_t::default();
        ensure(unsafe {
            sys::yunlink_v2_runtime_publish(
                self.raw(),
                string_view(&peer.id),
                session_id,
                family as u8,
                operation,
                native_target,
                native_type,
                native_payload,
                correlation_id,
                ttl_ms,
                qos as u8,
                string_view(source_entity_uid),
                &mut handle,
            )
        })?;
        Ok(MessageHandle {
            session_id: handle.session_id,
            message_id: handle.message_id,
            correlation_id: handle.correlation_id,
        })
    }

    pub fn session_has_profile(
        &self,
        peer: &Peer,
        session_id: u64,
        profile_id: &str,
        major: u16,
    ) -> bool {
        unsafe {
            sys::yunlink_v2_runtime_session_has_profile(
                self.raw(),
                string_view(&peer.id),
                session_id,
                string_view(profile_id),
                major,
            ) != 0
        }
    }
}

impl Drop for Runtime {
    fn drop(&mut self) {
        let raw = self.raw.lock().expect("v2 runtime mutex poisoned").0;
        unsafe {
            sys::yunlink_v2_runtime_unsubscribe(raw, self.callback_token);
            sys::yunlink_v2_runtime_stop(raw);
            sys::yunlink_v2_runtime_destroy(raw);
        }
        let _ = &self.callback_context;
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn public_contract_is_generic_and_owned() {
        let event = Event::Stream(Message {
            peer_id: "peer".into(),
            session_id: 1,
            operation: 4,
            qos: 2,
            message_id: 2,
            correlation_id: 0,
            created_at_ms: 3,
            ttl_ms: 100,
            source_endpoint_uid: "endpoint".into(),
            source_entity_uid: "entity".into(),
            target: Target::Broadcast,
            type_ref: TypeRef::new("example.profile", 1, "Example"),
            payload: vec![1, 2, 3],
        });
        let Event::Stream(message) = event else {
            panic!("expected stream")
        };
        assert_eq!(message.payload, [1, 2, 3]);
    }
}
