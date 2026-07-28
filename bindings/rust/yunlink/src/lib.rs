//! Safe, owned Rust SDK for YunLink Wire v2.

pub mod configuration;
pub mod configuration_client;
pub mod configuration_codec;
mod configuration_codec_io;
pub mod configuration_variants;
pub mod v2;

pub use configuration::*;
pub use configuration_client::{ConfigurationClient, ConfigurationEndpoint};
pub use configuration_codec::ConfigurationPayload;
pub use configuration_variants::*;
pub use v2::{
    Error, Event, Family, Message, MessageHandle, Peer, Profile, Qos, Result, Runtime,
    RuntimeConfig, Target, TypeRef,
};
