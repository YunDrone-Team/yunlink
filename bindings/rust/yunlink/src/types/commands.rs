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
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct TakeoffCommand;

/// Safe land command payload.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct LandCommand;

/// Safe return command payload.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub struct ReturnCommand;

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

/// Complete UAV control payload accepted by the current ROS Bridge.
#[derive(Debug, Clone, Copy, PartialEq)]
pub struct UavControlCommand {
    pub control_cmd: u8,
    pub desired_position: [f32; 3],
    pub desired_velocity: [f32; 3],
    pub desired_acceleration: [f32; 3],
    pub desired_body_xy_position: [f32; 2],
    pub desired_body_xy_velocity: [f32; 2],
    pub fixed_height_m: f32,
    pub yaw_mode: u8,
    pub desired_yaw_rad: f32,
    pub desired_yaw_rate_radps: f32,
    pub controller_type: u8,
}

impl Default for UavControlCommand {
    fn default() -> Self {
        Self {
            control_cmd: 0,
            desired_position: [0.0; 3],
            desired_velocity: [0.0; 3],
            desired_acceleration: [0.0; 3],
            desired_body_xy_position: [0.0; 2],
            desired_body_xy_velocity: [0.0; 2],
            fixed_height_m: 0.0,
            yaw_mode: 0,
            desired_yaw_rad: 0.0,
            desired_yaw_rate_radps: 0.0,
            controller_type: 0,
        }
    }
}

/// Complete ground-vehicle control payload accepted by a UGV Bridge entity.
#[derive(Debug, Clone, Copy, Default, PartialEq)]
pub struct UgvControlCommand {
    pub control_cmd: u8,
    pub desired_position: [f32; 3],
    pub desired_velocity: [f32; 3],
    pub body_linear_velocity: [f32; 3],
    pub body_angular_velocity: [f32; 3],
    pub desired_yaw_rad: f32,
    pub desired_wgs84_position: [f64; 3],
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

/// Local odometry snapshot published by a vehicle localization stack.
#[derive(Debug, Clone, PartialEq)]
pub struct LocalOdom {
    pub source_stamp_ns: u64,
    pub frame_id: String,
    pub child_frame_id: String,
    pub x_m: f32,
    pub y_m: f32,
    pub z_m: f32,
    pub orientation_x: f32,
    pub orientation_y: f32,
    pub orientation_z: f32,
    pub orientation_w: f32,
    pub vx_mps: f32,
    pub vy_mps: f32,
    pub vz_mps: f32,
    pub angular_x_radps: f32,
    pub angular_y_radps: f32,
    pub angular_z_radps: f32,
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
