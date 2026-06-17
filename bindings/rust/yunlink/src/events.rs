use yunlink_sys as sys;

use crate::ffi_util::string_from_c_buf;

/// Command kind decoded from `yunlink_command_result_event_t`.
///
/// Unknown values are preserved instead of failing the whole event parse, which
/// keeps older Rust SDKs tolerant of newer C ABI command kinds.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommandKind {
    /// Runtime did not classify the command kind.
    Unknown,
    /// Takeoff command.
    Takeoff,
    /// Land command.
    Land,
    /// Return-to-home command.
    Return,
    /// Goto position command.
    Goto,
    /// Velocity setpoint command.
    VelocitySetpoint,
    /// Trajectory chunk command.
    TrajectoryChunk,
    /// Formation task command.
    FormationTask,
    /// Unknown future or vendor-specific value.
    Other(u16),
}

impl CommandKind {
    pub(crate) fn from_native(value: u16) -> Self {
        match value {
            sys::YUNLINK_COMMAND_KIND_UNKNOWN => Self::Unknown,
            sys::YUNLINK_COMMAND_KIND_TAKEOFF => Self::Takeoff,
            sys::YUNLINK_COMMAND_KIND_LAND => Self::Land,
            sys::YUNLINK_COMMAND_KIND_RETURN => Self::Return,
            sys::YUNLINK_COMMAND_KIND_GOTO => Self::Goto,
            sys::YUNLINK_COMMAND_KIND_VELOCITY_SETPOINT => Self::VelocitySetpoint,
            sys::YUNLINK_COMMAND_KIND_TRAJECTORY_CHUNK => Self::TrajectoryChunk,
            sys::YUNLINK_COMMAND_KIND_FORMATION_TASK => Self::FormationTask,
            other => Self::Other(other),
        }
    }
}

/// Command lifecycle phase decoded from `yunlink_command_result_event_t`.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum CommandPhase {
    /// Command was received by the runtime or executor.
    Received,
    /// Command was accepted for execution.
    Accepted,
    /// Command execution is in progress.
    InProgress,
    /// Command execution succeeded.
    Succeeded,
    /// Command execution failed.
    Failed,
    /// Command execution was cancelled.
    Cancelled,
    /// Command expired before completion.
    Expired,
    /// Unknown future or vendor-specific value.
    Other(u8),
}

impl CommandPhase {
    pub(crate) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_COMMAND_PHASE_RECEIVED => Self::Received,
            sys::YUNLINK_COMMAND_PHASE_ACCEPTED => Self::Accepted,
            sys::YUNLINK_COMMAND_PHASE_IN_PROGRESS => Self::InProgress,
            sys::YUNLINK_COMMAND_PHASE_SUCCEEDED => Self::Succeeded,
            sys::YUNLINK_COMMAND_PHASE_FAILED => Self::Failed,
            sys::YUNLINK_COMMAND_PHASE_CANCELLED => Self::Cancelled,
            sys::YUNLINK_COMMAND_PHASE_EXPIRED => Self::Expired,
            other => Self::Other(other),
        }
    }
}

/// Safe command-result event.
///
/// This is parsed from the tagged C union inside `yunlink_runtime_event_t`.
#[derive(Debug, Clone, PartialEq)]
pub struct CommandResultEvent {
    /// Session that produced the result.
    pub session_id: u64,
    /// Result message id.
    pub message_id: u64,
    /// Correlation id matching the original command handle.
    pub correlation_id: u64,
    /// Command kind reported by the runtime.
    pub command_kind: CommandKind,
    /// Command phase reported by the runtime.
    pub phase: CommandPhase,
    /// Stable protocol/runtime result code.
    pub result_code: u16,
    /// Progress percentage, when provided by the runtime or executor.
    pub progress_percent: u8,
    /// Human-readable result detail copied from the fixed C buffer.
    pub detail: String,
}

/// Safe vehicle-core-state event.
#[derive(Debug, Clone, PartialEq)]
pub struct VehicleCoreStateEvent {
    /// Session carrying the state.
    pub session_id: u64,
    /// State message id.
    pub message_id: u64,
    /// State correlation id.
    pub correlation_id: u64,
    /// Armed flag.
    pub armed: bool,
    /// Battery percentage.
    pub battery_percent: f32,
}

/// Safe link event.
#[derive(Debug, Clone, PartialEq)]
pub struct LinkEvent {
    /// Peer id copied from the fixed C buffer.
    pub peer_id: String,
    /// Whether the link is up.
    pub is_up: bool,
}

/// Safe runtime error event.
#[derive(Debug, Clone, PartialEq)]
pub struct ErrorEvent {
    /// Stable error code.
    pub code: u16,
    /// Human-readable message copied from the fixed C buffer.
    pub message: String,
}

/// Safe runtime event enum exposed to Rust callers.
#[derive(Debug, Clone, PartialEq)]
pub enum Event {
    /// Link state changed.
    Link(LinkEvent),
    /// Runtime or transport error.
    Error(ErrorEvent),
    /// Command result arrived.
    CommandResult(CommandResultEvent),
    /// Vehicle core state arrived.
    VehicleCoreState(VehicleCoreStateEvent),
}

/// Broadcast channel capacity used by the Rust adapter.
pub const EVENT_CHANNEL_CAPACITY: usize = 64;

/// Convert one raw C ABI event into a safe Rust event.
///
/// The raw C type is a tagged union. This function is the single place in the
/// safe crate where that union is inspected and copied into owned Rust values.
pub(crate) fn parse_event(event: sys::yunlink_runtime_event_t) -> Option<Event> {
    match event.type_ {
        sys::YUNLINK_RUNTIME_EVENT_LINK => {
            let data = unsafe { event.data.link };
            Some(Event::Link(LinkEvent {
                peer_id: string_from_c_buf(&data.peer_id),
                is_up: data.is_up != 0,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_ERROR => {
            let data = unsafe { event.data.error };
            Some(Event::Error(ErrorEvent {
                code: data.code,
                message: string_from_c_buf(&data.message),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_COMMAND_RESULT => {
            let data = unsafe { event.data.command_result };
            Some(Event::CommandResult(CommandResultEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                command_kind: CommandKind::from_native(data.command_kind),
                phase: CommandPhase::from_native(data.phase),
                result_code: data.result_code,
                progress_percent: data.progress_percent,
                detail: string_from_c_buf(&data.detail),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE => {
            let data = unsafe { event.data.vehicle_core_state };
            Some(Event::VehicleCoreState(VehicleCoreStateEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                armed: data.armed != 0,
                battery_percent: data.battery_percent,
            }))
        }
        _ => None,
    }
}
