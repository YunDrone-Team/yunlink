//! UI-domain model for the Rust Advanced Monitor prototype.
//!
//! Values in this module are intentionally plain Rust data. They are safe to
//! clone, render, and test, and they do not contain C ABI handles or pointers.

mod config;
mod state;

pub use config::MonitorConfig;
pub use state::{MonitorState, StateSnapshotRow};

/// Left-navigation pages in the prototype UI.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MonitorPage {
    /// Command publishing and command history page.
    Commands,
    /// System-service placeholder page.
    System,
    /// State snapshot page.
    State,
    /// Runtime log page.
    Logs,
    /// ABI teaching page.
    Abi,
}

/// Minimal log severity used by the Logs page.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LogLevel {
    /// Informational runtime or UI message.
    Info,
    /// Non-fatal warning.
    Warn,
    /// Error surfaced by the runtime worker or SDK.
    Error,
}

/// UI-level command lifecycle derived from command-result phases.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommandLifecycle {
    /// Command publish returned a handle, but no result event has arrived yet.
    Sent,
    /// A command result event has arrived, but it is not terminal.
    Received,
    /// Command completed successfully.
    Succeeded,
    /// Command failed.
    Failed,
    /// Command was cancelled.
    Cancelled,
    /// Command expired.
    Expired,
}
