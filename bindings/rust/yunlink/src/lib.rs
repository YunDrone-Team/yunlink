//! Safe Rust SDK for the YunLink C ABI.
//!
//! Public callers use this crate instead of `yunlink-sys`. The SDK owns the
//! opaque runtime pointer, initializes `struct_size` fields, copies Rust strings
//! into fixed C buffers, parses tagged C unions into Rust enums, and maps raw
//! result codes into `YunlinkError`.

mod configuration;
mod error;
mod events;
mod ffi_util;
mod runtime;
mod runtime_logs;
mod runtime_system_service;
mod types;

pub use configuration::{
    ConfigApplyOutcome, ConfigApplyRequirement, ConfigChoice, ConfigEffects, ConfigFieldError,
    ConfigFieldSchema, ConfigFieldValue, ConfigResourceApplyResponse,
    ConfigResourceDescribeResponse, ConfigResourceDescriptor, ConfigResourceGetResponse,
    ConfigResourceListResponse, ConfigResourcePatchResponse, ConfigSnapshot, ConfigStatus,
    ConfigValue, ConfigurationHandle, ConfigurationResponse,
};
pub use error::{FfiErrorCode, Result, YunlinkError};
pub use events::{
    AuthorityStatusEvent, CommandKind, CommandPhase, CommandResultEvent, ErrorEvent, Event,
    FeatureGetEvent, FeatureListEvent, FeatureStartEvent, HostSystemEvent, LinkEvent,
    LocalOdomEvent, Px4StateEvent, TopicDescriptor, TopicListEvent, TopicSampleEvent,
    TopicSubscriptionEvent, VehicleCoreStateEvent, EVENT_CHANNEL_CAPACITY,
};
pub use runtime::Runtime;
pub use runtime_logs::{
    RuntimeLogListResponse, RuntimeLogReadResponse, RuntimeLogResponse, RuntimeLogSummary,
};
pub use types::{
    AgentType, AuthorityLease, AuthorityState, CommandHandle, ControlSource, GotoCommand,
    LandCommand, LocalOdom, PeerConnection, ReturnCommand, RuntimeConfig, Session, SessionInfo,
    SessionState, TakeoffCommand, TargetSelector, UavControlCommand, VehicleCoreState,
    VelocitySetpointCommand,
};

#[cfg(test)]
mod tests {
    use super::{CommandKind, CommandPhase, Event, FfiErrorCode, Px4StateEvent};
    use crate::events;
    use yunlink_sys as sys;

    #[test]
    fn public_api_exports_px4_state_event() {
        let event = Px4StateEvent {
            session_id: 1,
            message_id: 2,
            correlation_id: 3,
            source_id: 4,
            connected: true,
            armed: false,
            flight_mode: "POSCTL".to_string(),
            system_status: 8,
            landed_state: 1,
            battery_voltage_v: 16.2,
            battery_current_a: 1.5,
            battery_percentage: 0.76,
            local_x_m: 1.0,
            local_y_m: 2.0,
            local_z_m: 3.0,
            local_vx_mps: 0.1,
            local_vy_mps: 0.2,
            local_vz_mps: 0.3,
            local_yaw_rad: 0.5,
            local_orientation_x: 0.1,
            local_orientation_y: 0.2,
            local_orientation_z: 0.3,
            local_orientation_w: 0.9,
            target_x_m: 4.0,
            target_y_m: 5.0,
            target_z_m: 6.0,
            target_yaw_rad: 0.75,
            target_valid: true,
        };

        assert!(event.connected);
        assert_eq!(event.source_id, 4);
        assert_eq!(event.landed_state, 1);
        assert_eq!(event.local_orientation_w, 0.9);
    }

    #[test]
    fn ffi_error_code_mapping_is_complete_for_stable_result_set() {
        let stable_pairs = [
            (
                sys::YUNLINK_RESULT_INVALID_ARGUMENT,
                FfiErrorCode::InvalidArgument,
            ),
            (sys::YUNLINK_RESULT_SOCKET_ERROR, FfiErrorCode::SocketError),
            (sys::YUNLINK_RESULT_BIND_ERROR, FfiErrorCode::BindError),
            (sys::YUNLINK_RESULT_LISTEN_ERROR, FfiErrorCode::ListenError),
            (
                sys::YUNLINK_RESULT_CONNECT_ERROR,
                FfiErrorCode::ConnectError,
            ),
            (sys::YUNLINK_RESULT_TIMEOUT, FfiErrorCode::Timeout),
            (sys::YUNLINK_RESULT_ENCODE_ERROR, FfiErrorCode::EncodeError),
            (sys::YUNLINK_RESULT_DECODE_ERROR, FfiErrorCode::DecodeError),
            (
                sys::YUNLINK_RESULT_CHECKSUM_MISMATCH,
                FfiErrorCode::ChecksumMismatch,
            ),
            (
                sys::YUNLINK_RESULT_INVALID_HEADER,
                FfiErrorCode::InvalidHeader,
            ),
            (
                sys::YUNLINK_RESULT_RUNTIME_STOPPED,
                FfiErrorCode::RuntimeStopped,
            ),
            (
                sys::YUNLINK_RESULT_PROTOCOL_MISMATCH,
                FfiErrorCode::ProtocolMismatch,
            ),
            (sys::YUNLINK_RESULT_UNAUTHORIZED, FfiErrorCode::Unauthorized),
            (sys::YUNLINK_RESULT_REJECTED, FfiErrorCode::Rejected),
            (sys::YUNLINK_RESULT_INTERNAL, FfiErrorCode::Internal),
            (sys::YUNLINK_RESULT_NOT_FOUND, FfiErrorCode::NotFound),
        ];

        for (raw, expected) in stable_pairs {
            assert_eq!(FfiErrorCode::from_raw(raw), expected, "raw={raw}");
            assert!(
                FfiErrorCode::from_raw(raw)
                    .to_string()
                    .starts_with("YUNLINK_RESULT_"),
                "raw={raw}"
            );
        }

        assert_eq!(FfiErrorCode::from_raw(999), FfiErrorCode::Unknown(999));
        assert_eq!(
            FfiErrorCode::Unknown(999).to_string(),
            "YUNLINK_RESULT_UNKNOWN(999)"
        );
    }

    #[test]
    fn command_result_event_preserves_ffi_metadata() {
        let mut raw = sys::yunlink_runtime_event_t {
            type_: sys::YUNLINK_RUNTIME_EVENT_COMMAND_RESULT,
            data: sys::yunlink_runtime_event_union_t {
                command_result: sys::yunlink_command_result_event_t {
                    session_id: 42,
                    message_id: 9001,
                    correlation_id: 7001,
                    command_kind: sys::YUNLINK_COMMAND_KIND_GOTO,
                    phase: sys::YUNLINK_COMMAND_PHASE_FAILED,
                    result_code: sys::YUNLINK_RESULT_UNAUTHORIZED as u16,
                    progress_percent: 20,
                    detail: [0; 256],
                },
            },
        };
        let bytes = b"no-authority";
        unsafe {
            for (index, byte) in bytes.iter().enumerate() {
                raw.data.command_result.detail[index] = *byte as std::ffi::c_char;
            }
        }

        let Event::CommandResult(event) = events::parse_event(raw).expect("command result") else {
            panic!("expected command result event");
        };
        assert_eq!(event.command_kind, CommandKind::Goto);
        assert_eq!(event.phase, CommandPhase::Failed);
        assert_eq!(event.result_code, sys::YUNLINK_RESULT_UNAUTHORIZED as u16);
        assert_eq!(event.detail, "no-authority");
    }
}
