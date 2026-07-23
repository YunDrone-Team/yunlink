use yunlink_sys as sys;

use crate::ffi_util::string_from_c_buf;
use crate::types::AuthorityState;

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
    UavControl,
    UgvControl,
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
            sys::YUNLINK_COMMAND_KIND_UAV_CONTROL => Self::UavControl,
            sys::YUNLINK_COMMAND_KIND_UGV_CONTROL => Self::UgvControl,
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
    /// Entity identity that produced the result.
    pub source_type: crate::AgentType,
    pub source_id: u32,
    pub source_role: crate::EndpointRole,
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
    /// Source vehicle agent type.
    pub source_type: crate::AgentType,
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
    pub source_type: crate::AgentType,
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
    pub local_yaw_rad: f32,
    pub local_orientation_x: f32,
    pub local_orientation_y: f32,
    pub local_orientation_z: f32,
    pub local_orientation_w: f32,
    pub target_x_m: f32,
    pub target_y_m: f32,
    pub target_z_m: f32,
    pub target_yaw_rad: f32,
    pub target_valid: bool,
}

/// Local odometry pose and twist reported by the vehicle localization stack.
#[derive(Debug, Clone, PartialEq)]
pub struct LocalOdomEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: crate::AgentType,
    pub source_id: u32,
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

#[derive(Debug, Clone, PartialEq)]
pub struct UgvControlCmdEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: crate::AgentType,
    pub source_id: u32,
    pub source_stamp_ns: u64,
    pub frame_id: String,
    pub cmd_source: u8,
    pub control_cmd: u8,
    pub desired_position: [f32; 3],
    pub desired_velocity: [f32; 3],
    pub body_linear_velocity: [f32; 3],
    pub body_angular_velocity: [f32; 3],
    pub desired_yaw_rad: f32,
    pub desired_wgs84_position: [f64; 3],
}

#[derive(Debug, Clone, PartialEq)]
pub struct UgvControlStateEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: crate::AgentType,
    pub source_id: u32,
    pub source_stamp_ns: u64,
    pub frame_id: String,
    pub agent_name: String,
    pub agent_id: u32,
    pub drive_type: u8,
    pub control_cmd_valid: bool,
    pub inside_geo_fence: bool,
    pub diagnostic_level: u8,
    pub diagnostic_message: String,
    pub fsm_state: u8,
    pub active_control_cmd: u8,
    pub odom_valid: bool,
    pub odom_position: [f32; 3],
    pub odom_velocity: [f32; 3],
    pub target_valid: bool,
    pub target_position: [f32; 3],
    pub target_yaw_rad: f32,
    pub controller_linear_velocity: [f32; 3],
    pub controller_angular_velocity: [f32; 3],
    pub geo_fence_min: [f32; 3],
    pub geo_fence_max: [f32; 3],
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

/// Authority lease status confirmed by the remote target.
#[derive(Debug, Clone, PartialEq)]
pub struct AuthorityStatusEvent {
    pub state: AuthorityState,
    pub session_id: u64,
    pub source_type: crate::AgentType,
    pub source_id: u32,
    pub source_role: crate::EndpointRole,
    pub lease_ttl_ms: u32,
    pub reason_code: u16,
    pub detail: String,
}

/// Safe owned host resource and active-component snapshot.
#[derive(Debug, Clone, PartialEq)]
pub struct HostSystemEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_id: u32,
    pub source_stamp_ns: u64,
    pub cpu_percent: f32,
    pub memory_percent: f32,
    pub sample_period_ms: u32,
    pub component_kind: String,
    pub active_components: Vec<String>,
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
    pub features: Vec<FeatureDescriptor>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FeatureDescriptor {
    pub name: String,
    pub display_name: String,
    pub group_name: String,
    pub group_display_name: String,
    pub description: String,
    pub core_feature: bool,
    pub example_feature: bool,
    pub basic_feature: bool,
    pub auto_start: bool,
    pub check_feature_state: bool,
    pub runtime_state: u8,
    pub runtime_error: String,
    pub depends_on: Vec<String>,
    pub start_preview_units: Vec<String>,
    pub start_preview_commands: Vec<String>,
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
    pub title: String,
    pub group: String,
    pub description: String,
    pub depends_on: Vec<String>,
    pub start_preview_units: Vec<String>,
    pub start_preview_commands: Vec<String>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct FeatureStartEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub feature_name: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct FeatureStopEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub feature_name: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TopicDescriptor {
    pub name: String,
    pub type_name: String,
    pub publisher_count: u32,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TopicListEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub revision: String,
    pub topics: Vec<TopicDescriptor>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TopicSubscriptionEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub subscribed: bool,
    pub max_rate_hz: f32,
    pub max_payload_bytes: u32,
    pub message: String,
    pub topic_name: String,
    pub type_name: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct TopicSampleEvent {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub source_type: crate::AgentType,
    pub source_id: u32,
    pub source_role: crate::EndpointRole,
    pub receive_time_ns: u64,
    pub sequence: u64,
    pub metadata_included: bool,
    pub data_truncated: bool,
    pub topic_name: String,
    pub type_name: String,
    pub type_hash: String,
    pub encoding: String,
    pub message_definition: String,
    pub data: Vec<u8>,
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
    LocalOdom(LocalOdomEvent),
    UgvControlCmd(UgvControlCmdEvent),
    UgvControlState(UgvControlStateEvent),
    AuthorityStatus(AuthorityStatusEvent),
    FeatureList(FeatureListEvent),
    FeatureGet(FeatureGetEvent),
    FeatureStart(FeatureStartEvent),
    FeatureStop(FeatureStopEvent),
    HostSystem(HostSystemEvent),
    TopicList(TopicListEvent),
    TopicSubscription(TopicSubscriptionEvent),
    TopicSample(TopicSampleEvent),
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

fn percent_decode_feature_field(raw: &str) -> Option<String> {
    let bytes = raw.as_bytes();
    let mut decoded = Vec::with_capacity(bytes.len());
    let mut index = 0;
    while index < bytes.len() {
        if bytes[index] == b'%' {
            let encoded = std::str::from_utf8(bytes.get(index + 1..index + 3)?).ok()?;
            decoded.push(u8::from_str_radix(encoded, 16).ok()?);
            index += 3;
        } else {
            decoded.push(bytes[index]);
            index += 1;
        }
    }
    String::from_utf8(decoded).ok()
}

fn feature_descriptors(raw: String) -> Vec<FeatureDescriptor> {
    raw.split('\x1e')
        .filter(|record| !record.is_empty())
        .filter_map(|record| {
            let fields = record
                .split('\x1f')
                .map(percent_decode_feature_field)
                .collect::<Option<Vec<_>>>()?;
            let fields: [String; 15] = fields.try_into().ok()?;
            Some(FeatureDescriptor {
                name: fields[0].clone(),
                display_name: fields[1].clone(),
                group_name: fields[2].clone(),
                group_display_name: fields[3].clone(),
                description: fields[4].clone(),
                core_feature: fields[5] == "1",
                example_feature: fields[6] == "1",
                basic_feature: fields[7] == "1",
                auto_start: fields[8] == "1",
                check_feature_state: fields[9] == "1",
                runtime_state: fields[10].parse().ok()?,
                runtime_error: fields[11].clone(),
                depends_on: line_list(fields[12].clone()),
                start_preview_units: line_list(fields[13].clone()),
                start_preview_commands: line_list(fields[14].clone()),
            })
        })
        .collect()
}

fn line_list(raw: String) -> Vec<String> {
    raw.lines()
        .map(str::trim)
        .filter(|value| !value.is_empty())
        .map(ToString::to_string)
        .collect()
}

fn topic_list(raw: String) -> Vec<TopicDescriptor> {
    raw.lines()
        .filter_map(|line| {
            let mut fields = line.split('\t');
            let name = fields.next()?.trim();
            let type_name = fields.next()?.trim();
            let publisher_count = fields.next()?.trim().parse().ok()?;
            if name.is_empty() || type_name.is_empty() {
                return None;
            }
            Some(TopicDescriptor {
                name: name.into(),
                type_name: type_name.into(),
                publisher_count,
            })
        })
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
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_role: crate::EndpointRole::from_native(data.source_role),
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
                source_type: crate::AgentType::from_native(data.source_type),
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
                source_type: crate::AgentType::from_native(data.source_type),
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
                local_yaw_rad: data.local_yaw_rad,
                local_orientation_x: data.local_orientation_x,
                local_orientation_y: data.local_orientation_y,
                local_orientation_z: data.local_orientation_z,
                local_orientation_w: data.local_orientation_w,
                target_x_m: data.target_x_m,
                target_y_m: data.target_y_m,
                target_z_m: data.target_z_m,
                target_yaw_rad: data.target_yaw_rad,
                target_valid: data.target_valid != 0,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_LOCAL_ODOM => {
            let data = unsafe { event.data.local_odom };
            Some(Event::LocalOdom(LocalOdomEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_stamp_ns: data.source_stamp_ns,
                frame_id: string_from_c_buf(&data.frame_id),
                child_frame_id: string_from_c_buf(&data.child_frame_id),
                x_m: data.x_m,
                y_m: data.y_m,
                z_m: data.z_m,
                orientation_x: data.orientation_x,
                orientation_y: data.orientation_y,
                orientation_z: data.orientation_z,
                orientation_w: data.orientation_w,
                vx_mps: data.vx_mps,
                vy_mps: data.vy_mps,
                vz_mps: data.vz_mps,
                angular_x_radps: data.angular_x_radps,
                angular_y_radps: data.angular_y_radps,
                angular_z_radps: data.angular_z_radps,
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_UGV_CONTROL_CMD => {
            let data = unsafe { event.data.ugv_control_cmd };
            Some(Event::UgvControlCmd(UgvControlCmdEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_stamp_ns: data.source_stamp_ns,
                frame_id: string_from_c_buf(&data.frame_id),
                cmd_source: data.cmd_source,
                control_cmd: data.control_cmd,
                desired_position: [
                    data.desired_position_x_m,
                    data.desired_position_y_m,
                    data.desired_position_z_m,
                ],
                desired_velocity: [
                    data.desired_velocity_x_mps,
                    data.desired_velocity_y_mps,
                    data.desired_velocity_z_mps,
                ],
                body_linear_velocity: [
                    data.body_linear_velocity_x_mps,
                    data.body_linear_velocity_y_mps,
                    data.body_linear_velocity_z_mps,
                ],
                body_angular_velocity: [
                    data.body_angular_velocity_x_radps,
                    data.body_angular_velocity_y_radps,
                    data.body_angular_velocity_z_radps,
                ],
                desired_yaw_rad: data.desired_yaw_rad,
                desired_wgs84_position: [
                    data.desired_wgs84_latitude_deg,
                    data.desired_wgs84_longitude_deg,
                    data.desired_wgs84_altitude_m,
                ],
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_UGV_CONTROL_STATE => {
            let data = unsafe { event.data.ugv_control_state };
            Some(Event::UgvControlState(UgvControlStateEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_stamp_ns: data.source_stamp_ns,
                frame_id: string_from_c_buf(&data.frame_id),
                agent_name: string_from_c_buf(&data.agent_name),
                agent_id: data.agent_id,
                drive_type: data.drive_type,
                control_cmd_valid: data.control_cmd_valid != 0,
                inside_geo_fence: data.inside_geo_fence != 0,
                diagnostic_level: data.diagnostic_level,
                diagnostic_message: string_from_c_buf(&data.diagnostic_message),
                fsm_state: data.fsm_state,
                active_control_cmd: data.active_control_cmd,
                odom_valid: data.odom_valid != 0,
                odom_position: [data.odom_x_m, data.odom_y_m, data.odom_z_m],
                odom_velocity: [data.odom_vx_mps, data.odom_vy_mps, data.odom_vz_mps],
                target_valid: data.target_valid != 0,
                target_position: [data.target_x_m, data.target_y_m, data.target_z_m],
                target_yaw_rad: data.target_yaw_rad,
                controller_linear_velocity: [
                    data.controller_linear_x_mps,
                    data.controller_linear_y_mps,
                    data.controller_linear_z_mps,
                ],
                controller_angular_velocity: [
                    data.controller_angular_x_radps,
                    data.controller_angular_y_radps,
                    data.controller_angular_z_radps,
                ],
                geo_fence_min: [
                    data.geo_fence_min_x_m,
                    data.geo_fence_min_y_m,
                    data.geo_fence_min_z_m,
                ],
                geo_fence_max: [
                    data.geo_fence_max_x_m,
                    data.geo_fence_max_y_m,
                    data.geo_fence_max_z_m,
                ],
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_AUTHORITY_STATUS => {
            let data = unsafe { event.data.authority_status };
            Some(Event::AuthorityStatus(AuthorityStatusEvent {
                state: AuthorityState::from_native(data.state),
                session_id: data.session_id,
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_role: crate::EndpointRole::from_native(data.source_role),
                lease_ttl_ms: data.lease_ttl_ms,
                reason_code: data.reason_code,
                detail: string_from_c_buf(&data.detail),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_HOST_SYSTEM => {
            let data = unsafe { event.data.host_system };
            Some(Event::HostSystem(HostSystemEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_id: data.source_id,
                source_stamp_ns: data.source_stamp_ns,
                cpu_percent: data.cpu_percent,
                memory_percent: data.memory_percent,
                sample_period_ms: data.sample_period_ms,
                component_kind: string_from_c_buf(&data.component_kind),
                active_components: line_list(string_from_c_buf(&data.active_components)),
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
                features: feature_descriptors(string_from_c_buf(&data.feature_descriptors)),
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
                title: string_from_c_buf(&data.title),
                group: string_from_c_buf(&data.group),
                description: string_from_c_buf(&data.description),
                depends_on: csv_list(string_from_c_buf(&data.depends_on)),
                start_preview_units: csv_list(string_from_c_buf(&data.start_preview_units)),
                start_preview_commands: csv_list(string_from_c_buf(&data.start_preview_commands)),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_FEATURE_START => {
            let data = unsafe { event.data.feature_start };
            Some(Event::FeatureStart(FeatureStartEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                message: string_from_c_buf(&data.message),
                feature_name: string_from_c_buf(&data.feature_name),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_FEATURE_STOP => {
            let data = unsafe { event.data.feature_stop };
            Some(Event::FeatureStop(FeatureStopEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                message: string_from_c_buf(&data.message),
                feature_name: string_from_c_buf(&data.feature_name),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_TOPIC_LIST => {
            let data = unsafe { event.data.topic_list };
            Some(Event::TopicList(TopicListEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                message: string_from_c_buf(&data.message),
                revision: string_from_c_buf(&data.revision),
                topics: topic_list(string_from_c_buf(&data.topics)),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_TOPIC_SUBSCRIPTION => {
            let data = unsafe { event.data.topic_subscription };
            Some(Event::TopicSubscription(TopicSubscriptionEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                success: data.success != 0,
                subscribed: data.subscribed != 0,
                max_rate_hz: data.max_rate_hz,
                max_payload_bytes: data.max_payload_bytes,
                message: string_from_c_buf(&data.message),
                topic_name: string_from_c_buf(&data.topic_name),
                type_name: string_from_c_buf(&data.type_name),
            }))
        }
        sys::YUNLINK_RUNTIME_EVENT_TOPIC_SAMPLE => {
            let data = unsafe { event.data.topic_sample };
            let size = (data.data_size as usize).min(data.data.len());
            Some(Event::TopicSample(TopicSampleEvent {
                session_id: data.session_id,
                message_id: data.message_id,
                correlation_id: data.correlation_id,
                source_type: crate::AgentType::from_native(data.source_type),
                source_id: data.source_id,
                source_role: crate::EndpointRole::from_native(data.source_role),
                receive_time_ns: data.receive_time_ns,
                sequence: data.sequence,
                metadata_included: data.metadata_included != 0,
                data_truncated: data.data_truncated != 0,
                topic_name: string_from_c_buf(&data.topic_name),
                type_name: string_from_c_buf(&data.type_name),
                type_hash: string_from_c_buf(&data.type_hash),
                encoding: string_from_c_buf(&data.encoding),
                message_definition: string_from_c_buf(&data.message_definition),
                data: data.data[..size].to_vec(),
            }))
        }
        _ => None,
    }
}

#[cfg(test)]
mod tests {
    use super::{parse_event, Event};
    use crate::AuthorityState;
    use yunlink_sys as sys;

    fn write_c_buf<const N: usize>(target: &mut [std::ffi::c_char; N], source: &[u8]) {
        for (target, source) in target.iter_mut().zip(source) {
            *target = *source as std::ffi::c_char;
        }
    }

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
                    local_yaw_rad: 1.25,
                    local_orientation_x: 0.1,
                    local_orientation_y: -0.2,
                    local_orientation_z: 0.3,
                    local_orientation_w: 0.9,
                    target_x_m: 4.0,
                    target_y_m: 5.0,
                    target_z_m: 6.0,
                    target_yaw_rad: -0.75,
                    target_valid: 1,
                },
            },
        };

        match parse_event(raw).expect("PX4 state event should parse") {
            Event::Px4State(event) => {
                assert_eq!(event.session_id, 11);
                assert_eq!(event.message_id, 12);
                assert_eq!(event.correlation_id, 13);
                assert_eq!(event.source_type, crate::AgentType::Uav);
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
                assert_eq!(event.local_yaw_rad, 1.25);
                assert_eq!(event.local_orientation_w, 0.9);
                assert_eq!(event.target_x_m, 4.0);
                assert_eq!(event.target_y_m, 5.0);
                assert_eq!(event.target_z_m, 6.0);
                assert_eq!(event.target_yaw_rad, -0.75);
                assert!(event.target_valid);
            }
            other => panic!("expected PX4 state event, got {other:?}"),
        }
    }

    #[test]
    fn parses_local_odom_event_from_c_abi_union() {
        let mut data = sys::yunlink_local_odom_event_t {
            session_id: 41,
            message_id: 42,
            correlation_id: 43,
            source_id: 7,
            source_stamp_ns: 1_234_567_890,
            x_m: 1.25,
            y_m: -2.5,
            z_m: 3.75,
            orientation_z: 0.5,
            orientation_w: 0.8660254,
            vx_mps: 0.1,
            vy_mps: -0.2,
            vz_mps: 0.3,
            angular_z_radps: 0.4,
            ..Default::default()
        };
        write_c_buf(&mut data.frame_id, b"odom");
        write_c_buf(&mut data.child_frame_id, b"base_link");
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_LOCAL_ODOM,
            data: sys::yunlink_runtime_event_union_t { local_odom: data },
        };

        match parse_event(raw).expect("local odom event should parse") {
            Event::LocalOdom(event) => {
                assert_eq!(event.session_id, 41);
                assert_eq!(event.message_id, 42);
                assert_eq!(event.correlation_id, 43);
                assert_eq!(event.source_id, 7);
                assert_eq!(event.source_stamp_ns, 1_234_567_890);
                assert_eq!(event.frame_id, "odom");
                assert_eq!(event.child_frame_id, "base_link");
                assert_eq!(event.x_m, 1.25);
                assert_eq!(event.y_m, -2.5);
                assert_eq!(event.z_m, 3.75);
                assert_eq!(event.orientation_w, 0.8660254);
                assert_eq!(event.vy_mps, -0.2);
                assert_eq!(event.angular_z_radps, 0.4);
            }
            other => panic!("expected local odom event, got {other:?}"),
        }
    }

    #[test]
    fn parses_authority_status_event_from_c_abi_union() {
        let mut detail = [0; 256];
        for (target, source) in detail.iter_mut().zip(b"authority granted") {
            *target = *source as std::ffi::c_char;
        }
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_AUTHORITY_STATUS,
            data: sys::yunlink_runtime_event_union_t {
                authority_status: sys::yunlink_authority_status_event_t {
                    state: sys::YUNLINK_AUTHORITY_STATE_CONTROLLER,
                    session_id: 42,
                    source_type: sys::YUNLINK_AGENT_TYPE_UAV,
                    source_id: 7,
                    source_role: sys::YUNLINK_ROLE_VEHICLE,
                    lease_ttl_ms: 3_000,
                    reason_code: 0,
                    detail,
                },
            },
        };

        match parse_event(raw).expect("authority status event should parse") {
            Event::AuthorityStatus(event) => {
                assert_eq!(event.state, AuthorityState::Controller);
                assert_eq!(event.session_id, 42);
                assert_eq!(event.lease_ttl_ms, 3_000);
                assert_eq!(event.reason_code, 0);
                assert_eq!(event.detail, "authority granted");
            }
            other => panic!("expected authority status event, got {other:?}"),
        }
    }

    #[test]
    fn parses_host_system_event_into_owned_values() {
        let mut component_kind = [0; 32];
        for (target, source) in component_kind.iter_mut().zip(b"ros1_node") {
            *target = *source as std::ffi::c_char;
        }
        let mut active_components = [0; 8192];
        for (target, source) in active_components
            .iter_mut()
            .zip(b"/gazebo\n/sunray_system\n/yunlink_ros_bridge_node")
        {
            *target = *source as std::ffi::c_char;
        }
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_HOST_SYSTEM,
            data: sys::yunlink_runtime_event_union_t {
                host_system: sys::yunlink_host_system_event_t {
                    session_id: 11,
                    message_id: 12,
                    correlation_id: 13,
                    source_id: 1,
                    source_stamp_ns: 123_456_789,
                    cpu_percent: 11.5,
                    memory_percent: 22.25,
                    sample_period_ms: 1_000,
                    component_kind,
                    active_components,
                },
            },
        };

        match parse_event(raw).expect("host system event should parse") {
            Event::HostSystem(event) => {
                assert_eq!(event.source_stamp_ns, 123_456_789);
                assert_eq!(event.cpu_percent, 11.5);
                assert_eq!(event.memory_percent, 22.25);
                assert_eq!(event.component_kind, "ros1_node");
                assert_eq!(
                    event.active_components,
                    ["/gazebo", "/sunray_system", "/yunlink_ros_bridge_node"]
                );
            }
            other => panic!("expected host system event, got {other:?}"),
        }
    }

    #[test]
    fn parses_complete_feature_descriptor_from_c_abi_union() {
        let mut data = sys::yunlink_feature_list_event_t {
            session_id: 21,
            message_id: 22,
            correlation_id: 23,
            success: 1,
            ..Default::default()
        };
        write_c_buf(&mut data.message, b"ok");
        write_c_buf(&mut data.feature_names, b"ugv_example_hold");
        write_c_buf(
            &mut data.feature_descriptors,
            b"ugv_example_hold\x1fUGV Hold\x1fsunray_ugv_control_example\x1fUGV examples\x1fHold safely\x1f0\x1f1\x1f0\x1f0\x1f1\x1f2\x1f\x1fsunray_ugv_control\x1fUGV Hold\x1froslaunch ugv_hold.launch",
        );
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_FEATURE_LIST,
            data: sys::yunlink_runtime_event_union_t { feature_list: data },
        };

        match parse_event(raw).expect("FeatureList event should parse") {
            Event::FeatureList(event) => {
                assert_eq!(event.features.len(), 1);
                let feature = &event.features[0];
                assert_eq!(feature.name, "ugv_example_hold");
                assert!(feature.example_feature);
                assert!(!feature.basic_feature);
                assert!(feature.check_feature_state);
                assert_eq!(feature.runtime_state, 2);
                assert_eq!(feature.depends_on, ["sunray_ugv_control"]);
                assert_eq!(
                    feature.start_preview_commands,
                    ["roslaunch ugv_hold.launch"]
                );
            }
            other => panic!("expected FeatureList event, got {other:?}"),
        }
    }

    #[test]
    fn parses_feature_get_title_from_c_abi_union() {
        let mut data = sys::yunlink_feature_get_event_t {
            session_id: 31,
            message_id: 32,
            correlation_id: 33,
            success: 1,
            ..Default::default()
        };
        write_c_buf(&mut data.name, b"communication");
        write_c_buf(&mut data.title, "通信模块".as_bytes());
        write_c_buf(&mut data.group, b"system");
        write_c_buf(
            &mut data.description,
            "[通信模块]：启动云链 ROS bridge".as_bytes(),
        );
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_FEATURE_GET,
            data: sys::yunlink_runtime_event_union_t { feature_get: data },
        };

        match parse_event(raw).expect("FeatureGet event should parse") {
            Event::FeatureGet(event) => {
                assert_eq!(event.name, "communication");
                assert_eq!(event.title, "通信模块");
                assert_eq!(event.group, "system");
                assert_eq!(event.description, "[通信模块]：启动云链 ROS bridge");
            }
            other => panic!("expected FeatureGet event, got {other:?}"),
        }
    }

    #[test]
    fn parses_feature_start_response_from_c_abi_union() {
        let mut data = sys::yunlink_feature_start_event_t {
            session_id: 41,
            message_id: 42,
            correlation_id: 43,
            success: 1,
            ..Default::default()
        };
        write_c_buf(&mut data.message, b"started");
        write_c_buf(&mut data.feature_name, b"communication");
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_FEATURE_START,
            data: sys::yunlink_runtime_event_union_t {
                feature_start: data,
            },
        };

        match parse_event(raw).expect("FeatureStart event should parse") {
            Event::FeatureStart(event) => {
                assert!(event.success);
                assert_eq!(event.correlation_id, 43);
                assert_eq!(event.feature_name, "communication");
                assert_eq!(event.message, "started");
            }
            other => panic!("expected FeatureStart event, got {other:?}"),
        }
    }

    #[test]
    fn parses_feature_stop_response_from_c_abi_union() {
        let mut data = sys::yunlink_feature_stop_event_t {
            session_id: 51,
            message_id: 52,
            correlation_id: 53,
            success: 1,
            ..Default::default()
        };
        write_c_buf(&mut data.message, b"stopped");
        write_c_buf(&mut data.feature_name, b"circle_velocity");
        let raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_FEATURE_STOP,
            data: sys::yunlink_runtime_event_union_t { feature_stop: data },
        };

        match parse_event(raw).expect("FeatureStop event should parse") {
            Event::FeatureStop(event) => {
                assert!(event.success);
                assert_eq!(event.correlation_id, 53);
                assert_eq!(event.feature_name, "circle_velocity");
                assert_eq!(event.message, "stopped");
            }
            other => panic!("expected FeatureStop event, got {other:?}"),
        }
    }

    #[test]
    fn parses_ugv_control_events_as_owned_values() {
        let mut command = sys::yunlink_ugv_control_cmd_event_t {
            session_id: 51,
            source_type: sys::YUNLINK_AGENT_TYPE_UGV,
            source_id: 3,
            cmd_source: 1,
            control_cmd: 5,
            body_linear_velocity_x_mps: 1.25,
            body_angular_velocity_z_radps: -0.5,
            desired_wgs84_latitude_deg: 22.5401,
            ..Default::default()
        };
        write_c_buf(&mut command.frame_id, b"base_link");
        let raw_command = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_UGV_CONTROL_CMD,
            data: sys::yunlink_runtime_event_union_t {
                ugv_control_cmd: command,
            },
        };
        match parse_event(raw_command).expect("UGV command event should parse") {
            Event::UgvControlCmd(event) => {
                assert_eq!(event.source_type, crate::AgentType::Ugv);
                assert_eq!(event.source_id, 3);
                assert_eq!(event.frame_id, "base_link");
                assert_eq!(event.body_linear_velocity[0], 1.25);
                assert_eq!(event.body_angular_velocity[2], -0.5);
                assert_eq!(event.desired_wgs84_position[0], 22.5401);
            }
            other => panic!("expected UGV command event, got {other:?}"),
        }

        let mut state = sys::yunlink_ugv_control_state_event_t {
            session_id: 52,
            source_type: sys::YUNLINK_AGENT_TYPE_UGV,
            source_id: 3,
            agent_id: 3,
            drive_type: 2,
            control_cmd_valid: 1,
            diagnostic_level: 1,
            fsm_state: 3,
            odom_valid: 1,
            odom_x_m: 4.5,
            target_valid: 1,
            target_x_m: 8.0,
            controller_angular_z_radps: 0.2,
            ..Default::default()
        };
        write_c_buf(&mut state.agent_name, b"ugv");
        write_c_buf(&mut state.diagnostic_message, b"lateral velocity ignored");
        let raw_state = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_UGV_CONTROL_STATE,
            data: sys::yunlink_runtime_event_union_t {
                ugv_control_state: state,
            },
        };
        match parse_event(raw_state).expect("UGV state event should parse") {
            Event::UgvControlState(event) => {
                assert_eq!(event.agent_name, "ugv");
                assert!(event.control_cmd_valid);
                assert_eq!(event.diagnostic_message, "lateral velocity ignored");
                assert_eq!(event.odom_position[0], 4.5);
                assert_eq!(event.target_position[0], 8.0);
                assert_eq!(event.controller_angular_velocity[2], 0.2);
            }
            other => panic!("expected UGV state event, got {other:?}"),
        }
    }

    #[test]
    fn parses_topic_stream_events_and_bounds_sample_copy() {
        let mut list = sys::yunlink_topic_list_event_t {
            session_id: 51,
            message_id: 52,
            correlation_id: 53,
            success: 1,
            ..Default::default()
        };
        write_c_buf(&mut list.message, b"topic list ready");
        write_c_buf(&mut list.revision, b"rev-7");
        write_c_buf(
            &mut list.topics,
            b"/uav/odom\tnav_msgs/Odometry\t1\n/uav/state\tsunray_msgs/Px4State\t1",
        );
        let raw_list = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_TOPIC_LIST,
            data: sys::yunlink_runtime_event_union_t { topic_list: list },
        };
        match parse_event(raw_list).expect("topic list event should parse") {
            Event::TopicList(event) => {
                assert!(event.success);
                assert_eq!(event.revision, "rev-7");
                assert_eq!(event.topics.len(), 2);
                assert_eq!(event.topics[0].publisher_count, 1);
                assert_eq!(event.topics[1].type_name, "sunray_msgs/Px4State");
            }
            other => panic!("expected TopicList event, got {other:?}"),
        }

        let mut subscription = sys::yunlink_topic_subscription_event_t {
            session_id: 51,
            correlation_id: 54,
            success: 1,
            subscribed: 1,
            max_rate_hz: 10.0,
            max_payload_bytes: 4096,
            ..Default::default()
        };
        write_c_buf(&mut subscription.topic_name, b"/uav/odom");
        write_c_buf(&mut subscription.type_name, b"nav_msgs/Odometry");
        let raw_subscription = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_TOPIC_SUBSCRIPTION,
            data: sys::yunlink_runtime_event_union_t {
                topic_subscription: subscription,
            },
        };
        match parse_event(raw_subscription).expect("topic subscription event should parse") {
            Event::TopicSubscription(event) => {
                assert!(event.subscribed);
                assert_eq!(event.topic_name, "/uav/odom");
                assert_eq!(event.max_payload_bytes, 4096);
            }
            other => panic!("expected TopicSubscription event, got {other:?}"),
        }

        let mut sample = sys::yunlink_topic_sample_event_t {
            session_id: 51,
            source_type: sys::YUNLINK_AGENT_TYPE_UAV,
            source_id: 7,
            source_role: sys::YUNLINK_ROLE_VEHICLE,
            receive_time_ns: 123,
            sequence: 9,
            metadata_included: 1,
            data_size: (sys::YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY as u32) + 100,
            ..Default::default()
        };
        write_c_buf(&mut sample.topic_name, b"/uav/odom");
        write_c_buf(&mut sample.type_name, b"nav_msgs/Odometry");
        sample.data[..4].copy_from_slice(b"odom");
        let raw_sample = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_TOPIC_SAMPLE,
            data: sys::yunlink_runtime_event_union_t {
                topic_sample: sample,
            },
        };
        match parse_event(raw_sample).expect("topic sample event should parse") {
            Event::TopicSample(event) => {
                assert_eq!(event.topic_name, "/uav/odom");
                assert_eq!(event.source_type, crate::AgentType::Uav);
                assert_eq!(event.source_id, 7);
                assert_eq!(event.source_role, crate::EndpointRole::Vehicle);
                assert_eq!(event.data.len(), sys::YUNLINK_TOPIC_SAMPLE_DATA_CAPACITY);
                assert_eq!(&event.data[..4], b"odom");
            }
            other => panic!("expected TopicSample event, got {other:?}"),
        }
    }
}
