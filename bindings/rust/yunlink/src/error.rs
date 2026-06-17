//! Error mapping for the safe Rust SDK.
//!
//! The C ABI reports failures as stable integer result codes. This module
//! converts those raw values into Rust errors while preserving unknown future
//! codes for forward compatibility.

use std::fmt;

use thiserror::Error;
use yunlink_sys as sys;

/// Safe representation of `yunlink_result_t` failures.
///
/// `YUNLINK_RESULT_OK` is not represented here because successful calls return
/// `Ok(())`. Unknown values are retained so newer C ABI libraries remain
/// diagnosable when used with an older Rust SDK.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FfiErrorCode {
    /// Raw result was `YUNLINK_RESULT_INVALID_ARGUMENT`.
    InvalidArgument,
    /// Raw result was `YUNLINK_RESULT_SOCKET_ERROR`.
    SocketError,
    /// Raw result was `YUNLINK_RESULT_BIND_ERROR`.
    BindError,
    /// Raw result was `YUNLINK_RESULT_LISTEN_ERROR`.
    ListenError,
    /// Raw result was `YUNLINK_RESULT_CONNECT_ERROR`.
    ConnectError,
    /// Raw result was `YUNLINK_RESULT_TIMEOUT`.
    Timeout,
    /// Raw result was `YUNLINK_RESULT_ENCODE_ERROR`.
    EncodeError,
    /// Raw result was `YUNLINK_RESULT_DECODE_ERROR`.
    DecodeError,
    /// Raw result was `YUNLINK_RESULT_CHECKSUM_MISMATCH`.
    ChecksumMismatch,
    /// Raw result was `YUNLINK_RESULT_INVALID_HEADER`.
    InvalidHeader,
    /// Raw result was `YUNLINK_RESULT_RUNTIME_STOPPED`.
    RuntimeStopped,
    /// Raw result was `YUNLINK_RESULT_PROTOCOL_MISMATCH`.
    ProtocolMismatch,
    /// Raw result was `YUNLINK_RESULT_UNAUTHORIZED`.
    Unauthorized,
    /// Raw result was `YUNLINK_RESULT_REJECTED`.
    Rejected,
    /// Raw result was `YUNLINK_RESULT_INTERNAL`.
    Internal,
    /// Raw result was `YUNLINK_RESULT_NOT_FOUND`.
    NotFound,
    /// Unknown raw C ABI result code.
    Unknown(sys::yunlink_result_t),
}

impl FfiErrorCode {
    /// Convert one raw C ABI result code into a safe Rust error code.
    pub(crate) fn from_raw(code: sys::yunlink_result_t) -> Self {
        match code {
            sys::YUNLINK_RESULT_INVALID_ARGUMENT => Self::InvalidArgument,
            sys::YUNLINK_RESULT_SOCKET_ERROR => Self::SocketError,
            sys::YUNLINK_RESULT_BIND_ERROR => Self::BindError,
            sys::YUNLINK_RESULT_LISTEN_ERROR => Self::ListenError,
            sys::YUNLINK_RESULT_CONNECT_ERROR => Self::ConnectError,
            sys::YUNLINK_RESULT_TIMEOUT => Self::Timeout,
            sys::YUNLINK_RESULT_ENCODE_ERROR => Self::EncodeError,
            sys::YUNLINK_RESULT_DECODE_ERROR => Self::DecodeError,
            sys::YUNLINK_RESULT_CHECKSUM_MISMATCH => Self::ChecksumMismatch,
            sys::YUNLINK_RESULT_INVALID_HEADER => Self::InvalidHeader,
            sys::YUNLINK_RESULT_RUNTIME_STOPPED => Self::RuntimeStopped,
            sys::YUNLINK_RESULT_PROTOCOL_MISMATCH => Self::ProtocolMismatch,
            sys::YUNLINK_RESULT_UNAUTHORIZED => Self::Unauthorized,
            sys::YUNLINK_RESULT_REJECTED => Self::Rejected,
            sys::YUNLINK_RESULT_INTERNAL => Self::Internal,
            sys::YUNLINK_RESULT_NOT_FOUND => Self::NotFound,
            other => Self::Unknown(other),
        }
    }

    /// Return the stable C ABI constant name for display and tests.
    fn as_name(self) -> &'static str {
        match self {
            Self::InvalidArgument => "YUNLINK_RESULT_INVALID_ARGUMENT",
            Self::SocketError => "YUNLINK_RESULT_SOCKET_ERROR",
            Self::BindError => "YUNLINK_RESULT_BIND_ERROR",
            Self::ListenError => "YUNLINK_RESULT_LISTEN_ERROR",
            Self::ConnectError => "YUNLINK_RESULT_CONNECT_ERROR",
            Self::Timeout => "YUNLINK_RESULT_TIMEOUT",
            Self::EncodeError => "YUNLINK_RESULT_ENCODE_ERROR",
            Self::DecodeError => "YUNLINK_RESULT_DECODE_ERROR",
            Self::ChecksumMismatch => "YUNLINK_RESULT_CHECKSUM_MISMATCH",
            Self::InvalidHeader => "YUNLINK_RESULT_INVALID_HEADER",
            Self::RuntimeStopped => "YUNLINK_RESULT_RUNTIME_STOPPED",
            Self::ProtocolMismatch => "YUNLINK_RESULT_PROTOCOL_MISMATCH",
            Self::Unauthorized => "YUNLINK_RESULT_UNAUTHORIZED",
            Self::Rejected => "YUNLINK_RESULT_REJECTED",
            Self::Internal => "YUNLINK_RESULT_INTERNAL",
            Self::NotFound => "YUNLINK_RESULT_NOT_FOUND",
            Self::Unknown(_) => "YUNLINK_RESULT_UNKNOWN",
        }
    }
}

impl fmt::Display for FfiErrorCode {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::Unknown(raw) => write!(f, "{}({raw})", self.as_name()),
            _ => f.write_str(self.as_name()),
        }
    }
}

/// Top-level safe SDK error type.
#[derive(Debug, Error)]
pub enum YunlinkError {
    /// Error returned by a C ABI function.
    #[error("{0}")]
    Ffi(FfiErrorCode),
    /// A Rust string contained an interior null byte before being passed to C.
    #[error(transparent)]
    Nul(#[from] std::ffi::NulError),
    /// Event channel closed before an event could be delivered.
    #[error("event channel closed")]
    EventChannelClosed,
}

/// Convenience result alias used by the safe SDK.
pub type Result<T> = std::result::Result<T, YunlinkError>;

/// Convert a raw C ABI result code into a Rust `Result`.
pub(crate) fn ensure(code: sys::yunlink_result_t) -> Result<()> {
    if code == sys::YUNLINK_RESULT_OK {
        return Ok(());
    }
    Err(YunlinkError::Ffi(FfiErrorCode::from_raw(code)))
}
