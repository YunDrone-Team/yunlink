use yunlink_sys as sys;

use super::AgentType;

/// Safe target selector.
///
/// This wraps `yunlink_target_selector_t` and ensures `struct_size` is always
/// initialized before the value crosses the C ABI.
#[derive(Debug, Clone, Copy)]
pub struct TargetSelector {
    pub(crate) raw: sys::yunlink_target_selector_t,
}

impl TargetSelector {
    /// Select one concrete entity by type and id.
    pub fn entity(agent_type: AgentType, entity_id: u32) -> Self {
        Self {
            raw: sys::yunlink_target_selector_t {
                struct_size: std::mem::size_of::<sys::yunlink_target_selector_t>(),
                scope: sys::YUNLINK_TARGET_SCOPE_ENTITY,
                target_type: agent_type.to_native(),
                entity_id,
                group_id: 0,
            },
        }
    }

    /// Broadcast to all entities of a type.
    pub fn broadcast(agent_type: AgentType) -> Self {
        Self {
            raw: sys::yunlink_target_selector_t {
                struct_size: std::mem::size_of::<sys::yunlink_target_selector_t>(),
                scope: sys::YUNLINK_TARGET_SCOPE_BROADCAST,
                target_type: agent_type.to_native(),
                entity_id: 0,
                group_id: 0,
            },
        }
    }
}

/// Safe takeoff command payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct TakeoffCommand {
    /// Relative takeoff height in meters.
    pub relative_height_m: f32,
    /// Maximum velocity in meters per second.
    pub max_velocity_mps: f32,
}

/// Safe land command payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct LandCommand {
    /// Maximum velocity in meters per second.
    pub max_velocity_mps: f32,
}

/// Safe return command payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct ReturnCommand {
    /// Loiter time before returning, in seconds.
    pub loiter_before_return_s: f32,
}

/// Safe goto command payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct GotoCommand {
    /// Target X coordinate in meters.
    pub x_m: f32,
    /// Target Y coordinate in meters.
    pub y_m: f32,
    /// Target Z coordinate in meters.
    pub z_m: f32,
    /// Target yaw in radians.
    pub yaw_rad: f32,
}

/// Safe velocity setpoint command payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VelocitySetpointCommand {
    /// X velocity in meters per second.
    pub vx_mps: f32,
    /// Y velocity in meters per second.
    pub vy_mps: f32,
    /// Z velocity in meters per second.
    pub vz_mps: f32,
    /// Yaw rate in radians per second.
    pub yaw_rate_radps: f32,
    /// Whether the setpoint is body-frame rather than world-frame.
    pub body_frame: bool,
}

/// Safe vehicle core state payload.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct VehicleCoreState {
    /// Whether the vehicle is armed.
    pub armed: bool,
    /// Vehicle navigation mode as a protocol-defined integer.
    pub nav_mode: u8,
    /// Position X in meters.
    pub x_m: f32,
    /// Position Y in meters.
    pub y_m: f32,
    /// Position Z in meters.
    pub z_m: f32,
    /// Velocity X in meters per second.
    pub vx_mps: f32,
    /// Velocity Y in meters per second.
    pub vy_mps: f32,
    /// Velocity Z in meters per second.
    pub vz_mps: f32,
    /// Battery percentage, normally in the range 0..=100.
    pub battery_percent: f32,
}

/// Safe command handle returned by command publish methods.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CommandHandle {
    /// Session that published the command.
    pub session_id: u64,
    /// Protocol message id assigned to the command.
    pub message_id: u64,
    /// Correlation id echoed by command-result events.
    pub correlation_id: u64,
}
