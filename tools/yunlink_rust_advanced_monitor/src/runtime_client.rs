//! Runtime worker used by the egui app.
//!
//! egui runs on the UI thread and should not block on network I/O. This module
//! owns the safe `yunlink::Runtime` on a background thread, accepts high-level
//! UI commands through a channel, and sends UI-ready updates back through a
//! second channel. It deliberately imports the safe `yunlink` crate only; raw
//! `yunlink-sys` calls remain inside the SDK layer.

mod actions;
mod events;
mod worker;

use std::sync::mpsc;
use std::thread;

use tokio::runtime::Runtime as TokioRuntime;
use yunlink::Event;

use crate::model::MonitorConfig;

/// Commands that the UI can ask the runtime worker to perform.
///
/// Each variant is expressed in domain terms rather than C ABI terms. The worker
/// translates these into safe SDK calls, and the safe SDK performs the actual C
/// ABI conversion.
#[derive(Debug, Clone)]
pub enum RuntimeCommand {
    /// Connect to the configured remote peer and open a YunLink session.
    Connect,
    /// Request control authority for the prototype UAV target.
    RequestAuthority,
    /// Release the current control authority for the prototype UAV target.
    ReleaseAuthority,
    /// Publish a typed takeoff command.
    Takeoff,
    /// Publish a typed land command.
    Land,
    /// Publish a typed return command.
    Return,
    /// Publish a typed goto command in the local frame used by the protocol.
    Goto {
        /// Target X coordinate in meters.
        x_m: f32,
        /// Target Y coordinate in meters.
        y_m: f32,
        /// Target Z coordinate in meters.
        z_m: f32,
        /// Target yaw in radians.
        yaw_rad: f32,
    },
    /// Publish a velocity setpoint command.
    Velocity {
        /// X velocity in meters per second.
        vx_mps: f32,
        /// Y velocity in meters per second.
        vy_mps: f32,
        /// Z velocity in meters per second.
        vz_mps: f32,
        /// Yaw rate in radians per second.
        yaw_rate_radps: f32,
        /// Whether the setpoint is body-frame rather than world-frame.
        body_frame: bool,
    },
    /// Ask the worker loop to exit. Dropping the safe runtime closes the C ABI handle.
    Shutdown,
}

/// Updates emitted by the runtime worker and consumed by the egui model.
///
/// These updates are already safe Rust values. The UI never receives raw C
/// buffers, raw handles, or unions.
#[derive(Debug, Clone)]
pub enum RuntimeUpdate {
    /// The local YunLink runtime started successfully.
    Started,
    /// The peer was connected and a session was opened.
    Connected {
        /// Stable peer id returned by the C ABI through the safe SDK.
        peer_id: String,
        /// Opened session id.
        session_id: u64,
    },
    /// Current local view of authority ownership.
    Authority {
        /// Display label for the authority state.
        state: String,
    },
    /// A command publish call succeeded and returned a protocol handle.
    CommandSent {
        /// Human-readable command name.
        action: String,
        /// Compact payload summary for the history table.
        detail: String,
        /// Safe wrapper around session/message/correlation ids.
        handle: yunlink::CommandHandle,
    },
    /// A runtime event parsed by the safe SDK from `yunlink_runtime_event_t`.
    Event(Event),
    /// Human-readable error surfaced from the safe SDK.
    Error(String),
    /// Non-fatal note, such as event receiver lag.
    Note(String),
}

/// UI-thread handle to the background runtime worker.
pub struct RuntimeClient {
    tx: mpsc::Sender<RuntimeCommand>,
    rx: mpsc::Receiver<RuntimeUpdate>,
}

impl RuntimeClient {
    /// Spawn the worker thread and start a local YunLink runtime.
    pub fn spawn(config: MonitorConfig) -> Self {
        let (command_tx, command_rx) = mpsc::channel();
        let (update_tx, update_rx) = mpsc::channel();

        thread::spawn(move || {
            // The current safe SDK exposes async methods, so the worker owns a
            // Tokio runtime. This keeps the egui thread synchronous and simple.
            let runtime = TokioRuntime::new().expect("tokio runtime");
            runtime.block_on(worker::run(config, command_rx, update_tx));
        });

        Self {
            tx: command_tx,
            rx: update_rx,
        }
    }

    /// Send a high-level command to the worker.
    pub fn send(&self, command: RuntimeCommand) {
        let _ = self.tx.send(command);
    }

    /// Drain all pending worker updates without blocking the UI frame.
    pub fn drain_updates(&self) -> Vec<RuntimeUpdate> {
        let mut updates = Vec::new();
        while let Ok(update) = self.rx.try_recv() {
            updates.push(update);
        }
        updates
    }
}
