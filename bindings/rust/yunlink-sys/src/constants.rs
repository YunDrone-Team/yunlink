//! Raw numeric constants mirrored from `include/yunlink/c/abi/enums.h`.
//!
//! The `yunlink-sys` crate intentionally keeps these values in the C ABI shape:
//! small integer types, C-style names, and no semantic conversion. The safe
//! `yunlink` crate is responsible for turning them into Rust enums and for
//! preserving unknown future values where that is useful.

/// Current stable C ABI version exported by `yunlink_ffi_abi_version`.
pub const YUNLINK_FFI_ABI_VERSION: u32 = 1;

/// Raw C ABI result code type.
pub type yunlink_result_t = i32;

/// Operation completed successfully.
pub const YUNLINK_RESULT_OK: yunlink_result_t = 0;
/// A null pointer, invalid struct size, malformed string, or invalid parameter was supplied.
pub const YUNLINK_RESULT_INVALID_ARGUMENT: yunlink_result_t = 1;
/// Runtime failed while creating or using a socket.
pub const YUNLINK_RESULT_SOCKET_ERROR: yunlink_result_t = 2;
/// Runtime failed to bind a socket.
pub const YUNLINK_RESULT_BIND_ERROR: yunlink_result_t = 3;
/// Runtime failed to listen on a socket.
pub const YUNLINK_RESULT_LISTEN_ERROR: yunlink_result_t = 4;
/// Runtime failed to connect to a remote peer.
pub const YUNLINK_RESULT_CONNECT_ERROR: yunlink_result_t = 5;
/// Operation timed out.
pub const YUNLINK_RESULT_TIMEOUT: yunlink_result_t = 6;
/// Protocol encoding failed before a frame could be sent.
pub const YUNLINK_RESULT_ENCODE_ERROR: yunlink_result_t = 7;
/// Protocol decoding failed while reading an incoming frame.
pub const YUNLINK_RESULT_DECODE_ERROR: yunlink_result_t = 8;
/// Incoming frame checksum did not match the payload.
pub const YUNLINK_RESULT_CHECKSUM_MISMATCH: yunlink_result_t = 9;
/// Incoming frame header was syntactically invalid.
pub const YUNLINK_RESULT_INVALID_HEADER: yunlink_result_t = 10;
/// Runtime was stopped or has not been started.
pub const YUNLINK_RESULT_RUNTIME_STOPPED: yunlink_result_t = 11;
/// Remote peer and local runtime negotiated incompatible protocol values.
pub const YUNLINK_RESULT_PROTOCOL_MISMATCH: yunlink_result_t = 12;
/// Command or request requires authority that the caller does not hold.
pub const YUNLINK_RESULT_UNAUTHORIZED: yunlink_result_t = 13;
/// Runtime or remote peer rejected the operation.
pub const YUNLINK_RESULT_REJECTED: yunlink_result_t = 14;
/// Internal runtime error.
pub const YUNLINK_RESULT_INTERNAL: yunlink_result_t = 15;
/// Requested item, session, peer, or event was not found.
pub const YUNLINK_RESULT_NOT_FOUND: yunlink_result_t = 100;

/// Ground station endpoint agent type.
pub const YUNLINK_AGENT_TYPE_GROUND_STATION: u8 = 1;
/// Unmanned aerial vehicle endpoint agent type.
pub const YUNLINK_AGENT_TYPE_UAV: u8 = 2;
/// Unmanned ground vehicle endpoint agent type.
pub const YUNLINK_AGENT_TYPE_UGV: u8 = 3;
/// Swarm controller endpoint agent type.
pub const YUNLINK_AGENT_TYPE_SWARM_CONTROLLER: u8 = 4;

/// Observer role; may inspect state but does not control a target.
pub const YUNLINK_ROLE_OBSERVER: u8 = 1;
/// Controller role; may request authority and publish commands.
pub const YUNLINK_ROLE_CONTROLLER: u8 = 2;
/// Vehicle role; represents an endpoint that executes commands and publishes state.
pub const YUNLINK_ROLE_VEHICLE: u8 = 3;

/// Target selector addresses one concrete entity.
pub const YUNLINK_TARGET_SCOPE_ENTITY: u8 = 1;
/// Target selector addresses a group.
pub const YUNLINK_TARGET_SCOPE_GROUP: u8 = 2;
/// Target selector broadcasts to all matching entities.
pub const YUNLINK_TARGET_SCOPE_BROADCAST: u8 = 3;

/// Authority request source for ground-station initiated control.
pub const YUNLINK_CONTROL_SOURCE_GROUND_STATION: u8 = 1;

/// No command authority is currently held.
pub const YUNLINK_AUTHORITY_STATE_OBSERVER: u8 = 0;
/// Authority request has been sent and is waiting for a grant decision.
pub const YUNLINK_AUTHORITY_STATE_PENDING_GRANT: u8 = 1;
/// Session currently owns command authority for the target.
pub const YUNLINK_AUTHORITY_STATE_CONTROLLER: u8 = 2;
/// Session is preempting another controller.
pub const YUNLINK_AUTHORITY_STATE_PREEMPTING: u8 = 3;
/// Previously held authority was revoked.
pub const YUNLINK_AUTHORITY_STATE_REVOKED: u8 = 4;
/// Authority was explicitly released.
pub const YUNLINK_AUTHORITY_STATE_RELEASED: u8 = 5;
/// Authority request was rejected.
pub const YUNLINK_AUTHORITY_STATE_REJECTED: u8 = 6;

/// Command was received by the runtime or executor.
pub const YUNLINK_COMMAND_PHASE_RECEIVED: u8 = 1;
/// Command was accepted for execution.
pub const YUNLINK_COMMAND_PHASE_ACCEPTED: u8 = 2;
/// Command execution is in progress.
pub const YUNLINK_COMMAND_PHASE_IN_PROGRESS: u8 = 3;
/// Command execution succeeded.
pub const YUNLINK_COMMAND_PHASE_SUCCEEDED: u8 = 4;
/// Command execution failed.
pub const YUNLINK_COMMAND_PHASE_FAILED: u8 = 5;
/// Command execution was cancelled.
pub const YUNLINK_COMMAND_PHASE_CANCELLED: u8 = 6;
/// Command expired before completion.
pub const YUNLINK_COMMAND_PHASE_EXPIRED: u8 = 7;

/// Unknown or not-yet-classified command kind.
pub const YUNLINK_COMMAND_KIND_UNKNOWN: u16 = 0;
/// Takeoff command kind.
pub const YUNLINK_COMMAND_KIND_TAKEOFF: u16 = 1;
/// Land command kind.
pub const YUNLINK_COMMAND_KIND_LAND: u16 = 2;
/// Return-to-home command kind.
pub const YUNLINK_COMMAND_KIND_RETURN: u16 = 3;
/// Goto position command kind.
pub const YUNLINK_COMMAND_KIND_GOTO: u16 = 4;
/// Velocity setpoint command kind.
pub const YUNLINK_COMMAND_KIND_VELOCITY_SETPOINT: u16 = 5;
/// Trajectory chunk command kind reserved for future monitor coverage.
pub const YUNLINK_COMMAND_KIND_TRAJECTORY_CHUNK: u16 = 6;
/// Formation task command kind reserved for future monitor coverage.
pub const YUNLINK_COMMAND_KIND_FORMATION_TASK: u16 = 7;

/// Session was discovered but has not started a handshake.
pub const YUNLINK_SESSION_STATE_DISCOVERED: u8 = 1;
/// Session is exchanging handshake data.
pub const YUNLINK_SESSION_STATE_HANDSHAKING: u8 = 2;
/// Session passed authentication.
pub const YUNLINK_SESSION_STATE_AUTHENTICATED: u8 = 3;
/// Session completed capability negotiation.
pub const YUNLINK_SESSION_STATE_NEGOTIATED: u8 = 4;
/// Session is active and usable.
pub const YUNLINK_SESSION_STATE_ACTIVE: u8 = 5;
/// Session is draining before close.
pub const YUNLINK_SESSION_STATE_DRAINING: u8 = 6;
/// Session closed normally.
pub const YUNLINK_SESSION_STATE_CLOSED: u8 = 7;
/// Session was lost unexpectedly.
pub const YUNLINK_SESSION_STATE_LOST: u8 = 8;
/// Session handle is invalid.
pub const YUNLINK_SESSION_STATE_INVALID: u8 = 9;

/// Polling produced no runtime event.
pub const YUNLINK_RUNTIME_EVENT_NONE: u8 = 0;
/// Runtime event payload is `yunlink_link_event_t`.
pub const YUNLINK_RUNTIME_EVENT_LINK: u8 = 1;
/// Runtime event payload is `yunlink_error_event_t`.
pub const YUNLINK_RUNTIME_EVENT_ERROR: u8 = 2;
/// Runtime event payload is `yunlink_command_result_event_t`.
pub const YUNLINK_RUNTIME_EVENT_COMMAND_RESULT: u8 = 3;
/// Runtime event payload is `yunlink_vehicle_core_state_event_t`.
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE: u8 = 4;
/// Runtime event payload is `yunlink_vehicle_event_data_t`.
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT: u8 = 5;
pub const YUNLINK_CONFIG_VALUE_BOOL: u8 = 1;
pub const YUNLINK_CONFIG_VALUE_INT64: u8 = 2;
pub const YUNLINK_CONFIG_VALUE_DOUBLE: u8 = 3;
pub const YUNLINK_CONFIG_VALUE_STRING: u8 = 4;
pub const YUNLINK_CONFIG_VALUE_STRING_LIST: u8 = 5;
pub const YUNLINK_CONFIG_VALUE_DOUBLE_LIST: u8 = 6;

pub const YUNLINK_CONFIG_STATUS_OK: u8 = 0;
pub const YUNLINK_CONFIG_STATUS_NOT_FOUND: u8 = 1;
pub const YUNLINK_CONFIG_STATUS_UNSUPPORTED: u8 = 2;
pub const YUNLINK_CONFIG_STATUS_UNAUTHENTICATED: u8 = 3;
pub const YUNLINK_CONFIG_STATUS_UNAUTHORIZED: u8 = 4;
pub const YUNLINK_CONFIG_STATUS_CONFLICT: u8 = 5;
pub const YUNLINK_CONFIG_STATUS_INVALID: u8 = 6;
pub const YUNLINK_CONFIG_STATUS_UNSAFE_STATE: u8 = 7;
pub const YUNLINK_CONFIG_STATUS_INTERNAL_ERROR: u8 = 8;

pub const YUNLINK_CONFIG_APPLY_NONE: u8 = 0;
pub const YUNLINK_CONFIG_APPLY_COMPONENT_RESTART: u8 = 1;
pub const YUNLINK_CONFIG_APPLY_ENDPOINT_RESTART: u8 = 2;
pub const YUNLINK_CONFIG_APPLY_DEVICE_REBOOT: u8 = 3;
pub const YUNLINK_CONFIG_APPLY_MANUAL: u8 = 4;

pub const YUNLINK_CONFIG_OUTCOME_APPLIED: u8 = 1;
pub const YUNLINK_CONFIG_OUTCOME_RESTART_SCHEDULED: u8 = 2;
pub const YUNLINK_CONFIG_OUTCOME_MANUAL_ACTION_REQUIRED: u8 = 3;
pub const YUNLINK_CONFIG_OUTCOME_FAILED: u8 = 4;

pub const YUNLINK_RUNTIME_EVENT_FEATURE_LIST: u8 = 6;
pub const YUNLINK_RUNTIME_EVENT_FEATURE_GET: u8 = 7;
pub const YUNLINK_RUNTIME_EVENT_PX4_STATE: u8 = 8;
/// Authority lease status response.
pub const YUNLINK_RUNTIME_EVENT_AUTHORITY_STATUS: u8 = 9;
/// Host CPU, memory, and active-component snapshot.
pub const YUNLINK_RUNTIME_EVENT_HOST_SYSTEM: u8 = 10;
