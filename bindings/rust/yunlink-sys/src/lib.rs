//! Raw Rust bindings for the YunLink C ABI.
//!
//! This crate is deliberately unsafe and literal: item names, integer values,
//! struct layout, fixed buffers, and extern function signatures mirror the C
//! headers. Application code should normally depend on the safe `yunlink` crate,
//! which owns pointer lifetimes and converts these raw ABI values into Rust
//! domain types.

#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]

mod configuration;
mod constants;
mod events;
mod functions;
mod runtime_logs;
mod types;

pub use configuration::*;
pub use constants::*;
pub use events::*;
pub use functions::*;
pub use runtime_logs::*;
pub use types::*;
