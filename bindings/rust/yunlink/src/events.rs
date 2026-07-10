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
    /// Source vehicle agent id.
    pub source_id: u32,
    /// Armed flag.
    pub armed: bool,
    /// Numeric navigation mode reported by the vehicle.
    pub nav_mode: u8,
    /// Local position in metres.
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    /// Local velocity in metres per second.
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    /// Battery percentage.
    pub battery_percent: f32,
}

/// Safe link event.
#[derive(Debug, Clone, PartialEq)]
pub struct Px4StateEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_id: u32,
    pub connected: bool,
    pub armed: bool,
    pub flight_mode: String,
    pub system_status: u8,
    pub landed_state: u8,
    pub battery_voltage_v: f32,
    pub battery_current_a: f32,
    pub battery_percentage: f32,
    pub local_x_m: f32,
    pub local_y_m: f32,
    pub local_z_m: f32,
    pub local_vx_mps: f32,
    pub local_vy_mps: f32,
    pub local_vz_mps: f32,
}

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
pub struct FeatureListEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub feature_names: Vec<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct FeatureGetEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub running: bool,
    pub auto_start: bool,
    pub message: String,
    pub name: String,
    pub group: String,
    pub description: String,
    pub depends_on: Vec<String>,
    pub start_preview_units: Vec<String>,
    pub start_preview_commands: Vec<String>,
}

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
    Px4State(Px4StateEvent),
    FeatureList(FeatureListEvent),
    FeatureGet(FeatureGetEvent),
}

/// Broadcast channel capacity used by the Rust adapter.
pub const EVENT_CHANNEL_CAPACITY: usize = 64;

fn csv_list(raw: String) -> Vec<String> {
    raw.split(',')
        .map(str::trim)
        .filter(|value| !value.is_empty())
        .map(ToString::to_string)
        .collect()
}

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
                source_id: data.source_id,
                armed: data.armed != 0,
                nav_mode: data.nav_mode,
                x_m: data.x_m,
                y_m: data.y_m,
                z_m: data.z_m,
                vx_mps: data.vx_mps,
                vy_mps: data.vy_mps,
                vz_mps: data.vz_mps,
                battery_percent: data.battery_percent,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_PX4_STATE => {
            let data = unsafe { event.data.px4_state };
            Some(Event::Px4State(Px4StateEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_id: data.source_id,
                connected: data.connected != 0,
                armed: data.armed != 0,
                flight_mode: string_from_c_buf(&data.flight_mode),
                system_status: data.system_status,
                landed_state: data.landed_state,
                battery_voltage_v: data.battery_voltage_v,
                battery_current_a: data.battery_current_a,
                battery_percentage: data.battery_percentage,
                local_x_m: data.local_x_m,
                local_y_m: data.local_y_m,
                local_z_m: data.local_z_m,
                local_vx_mps: data.local_vx_mps,
                local_vy_mps: data.local_vy_mps,
                local_vz_mps: data.local_vz_mps,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_FEATURE_LIST => {
            let data = unsafe { event.data.feature_list };
            Some(Event::FeatureList(FeatureListEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                message: string_from_c_buf(&data.message),
                feature_names: csv_list(string_from_c_buf(&data.feature_names)),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_FEATURE_GET => {
            let data = unsafe { event.data.feature_get };
            Some(Event::FeatureGet(FeatureGetEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                running: data.running != 0,
                auto_start: data.auto_start != 0,
                message: string_from_c_buf(&data.message),
                name: string_from_c_buf(&data.name),
                group: string_from_c_buf(&data.group),
                description: string_from_c_buf(&data.description),
                depends_on: csv_list(string_from_c_buf(&data.depends_on)),
                start_preview_units: csv_list(string_from_c_buf(&data.start_preview_units)),
                start_preview_commands: csv_list(string_from_c_buf(&data.start_preview_commands)),
            }))
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::{parse_event, Event};
    use yunlink_sys as sys;

    #[test]
    fn parses_px4_state_event_from_c_abi_union() {
        let mut flight_mode = [0; 32];
        for (target, source) in flight_mode.iter_mut().zip(b"OFFBOARD") {
            *target = *source as std::ffi::c_char;
        }
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_PX4_STATE,
            data: sys::yunlink_runtime_event_union_t {
                px4_state: sys::yunlink_px4_state_event_t {
                    session_id: 11,
                    message_id: 12,
                    correlation_id: 13,
                    source_type: 2,
                    source_id: 1,
                    source_role: 3,
                    connected: 1,
                    armed: 1,
                    flight_mode,
                    system_status: 5,
                    landed_state: 2,
                    battery_voltage_v: 16.8,
                    battery_current_a: 2.4,
                    battery_percentage: 0.82,
                    local_x_m: 1.25,
                    local_y_m: -2.5,
                    local_z_m: 3.75,
                    local_vx_mps: 0.1,
                    local_vy_mps: -0.2,
                    local_vz_mps: 0.3,
                },
            },
        };

        match parse_event(raw).expect("PX4 state event should parse") {
            Event::Px4State(event) => {
                assert_eq!(event.session_id, 11);
                assert_eq!(event.message_id, 12);
                assert_eq!(event.correlation_id, 13);
                assert_eq!(event.source_id, 1);
                assert!(event.connected);
                assert!(event.armed);
                assert_eq!(event.flight_mode, "OFFBOARD");
                assert_eq!(event.system_status, 5);
                assert_eq!(event.landed_state, 2);
                assert_eq!(event.battery_voltage_v, 16.8);
                assert_eq!(event.battery_current_a, 2.4);
                assert_eq!(event.battery_percentage, 0.82);
                assert_eq!(event.local_x_m, 1.25);
                assert_eq!(event.local_y_m, -2.5);
                assert_eq!(event.local_z_m, 3.75);
                assert_eq!(event.local_vx_mps, 0.1);
                assert_eq!(event.local_vy_mps, -0.2);
                assert_eq!(event.local_vz_mps, 0.3);
            }
            other => panic!("expected PX4 state event, got {other:?}"),
        }
    }
}
