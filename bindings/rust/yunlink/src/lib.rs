//! Safe, owned Rust SDK for YunLink Wire v2.

pub mod v2;

pub use v2::{
    Error, Event, Family, Message, MessageHandle, Peer, Profile, Qos, Result, Runtime,
    RuntimeConfig, Target, TypeRef,
};
