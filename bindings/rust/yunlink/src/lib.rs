mod error;
mod events;
mod ffi_util;
mod runtime;
mod types;

pub use error::{FfiErrorCode, Result, YunlinkError};
pub use events::{
    CommandResultEvent, ErrorEvent, Event, LinkEvent, VehicleCoreStateEvent,
    EVENT_CHANNEL_CAPACITY,
};
pub use runtime::Runtime;
pub use types::{
    AgentType, AuthorityLease, AuthorityState, CommandHandle, ControlSource, GotoCommand,
    PeerConnection, RuntimeConfig, Session, TargetSelector, VehicleCoreState,
};

#[cfg(test)]
mod tests {
    use super::FfiErrorCode;
    use yunlink_sys as sys;

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
            (sys::YUNLINK_RESULT_CONNECT_ERROR, FfiErrorCode::ConnectError),
            (sys::YUNLINK_RESULT_TIMEOUT, FfiErrorCode::Timeout),
            (sys::YUNLINK_RESULT_ENCODE_ERROR, FfiErrorCode::EncodeError),
            (sys::YUNLINK_RESULT_DECODE_ERROR, FfiErrorCode::DecodeError),
            (
                sys::YUNLINK_RESULT_CHECKSUM_MISMATCH,
                FfiErrorCode::ChecksumMismatch,
            ),
            (sys::YUNLINK_RESULT_INVALID_HEADER, FfiErrorCode::InvalidHeader),
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
}
