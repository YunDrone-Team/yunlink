use std::fmt;

use thiserror::Error;
use yunlink_sys as sys;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum FfiErrorCode {
    InvalidArgument,
    SocketError,
    BindError,
    ListenError,
    ConnectError,
    Timeout,
    EncodeError,
    DecodeError,
    ChecksumMismatch,
    InvalidHeader,
    RuntimeStopped,
    ProtocolMismatch,
    Unauthorized,
    Rejected,
    Internal,
    NotFound,
    Unknown(sys::yunlink_result_t),
}

impl FfiErrorCode {
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

#[derive(Debug, Error)]
pub enum YunlinkError {
    #[error("{0}")]
    Ffi(FfiErrorCode),
    #[error(transparent)]
    Nul(#[from] std::ffi::NulError),
    #[error("event channel closed")]
    EventChannelClosed,
}

pub type Result<T> = std::result::Result<T, YunlinkError>;

pub(crate) fn ensure(code: sys::yunlink_result_t) -> Result<()> {
    if code == sys::YUNLINK_RESULT_OK {
        return Ok(());
    }
    Err(YunlinkError::Ffi(FfiErrorCode::from_raw(code)))
}
