//! Owned configuration resource types and C callback conversion.

mod callbacks;
mod native_patch;
mod types;
mod views;

pub use types::*;

pub(crate) use callbacks::{register_callbacks, ConfigurationCallbackContext};
pub(crate) use native_patch::{string_view, NativePatch};
