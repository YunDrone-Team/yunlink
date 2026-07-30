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
    Link {
        peer_id: String,
        up: bool,
    },
    Error {
        peer_id: String,
        code: u16,
        message: String,
    },
}

include!("v2/callback.rs");
include!("v2/runtime.rs");

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
