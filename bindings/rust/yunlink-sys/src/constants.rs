pub const YUNLINK_FFI_ABI_VERSION: u32 = 1;

pub type yunlink_result_t = i32;

pub const YUNLINK_RESULT_OK: yunlink_result_t = 0;
pub const YUNLINK_RESULT_INVALID_ARGUMENT: yunlink_result_t = 1;
pub const YUNLINK_RESULT_SOCKET_ERROR: yunlink_result_t = 2;
pub const YUNLINK_RESULT_BIND_ERROR: yunlink_result_t = 3;
pub const YUNLINK_RESULT_LISTEN_ERROR: yunlink_result_t = 4;
pub const YUNLINK_RESULT_CONNECT_ERROR: yunlink_result_t = 5;
pub const YUNLINK_RESULT_TIMEOUT: yunlink_result_t = 6;
pub const YUNLINK_RESULT_ENCODE_ERROR: yunlink_result_t = 7;
pub const YUNLINK_RESULT_DECODE_ERROR: yunlink_result_t = 8;
pub const YUNLINK_RESULT_CHECKSUM_MISMATCH: yunlink_result_t = 9;
pub const YUNLINK_RESULT_INVALID_HEADER: yunlink_result_t = 10;
pub const YUNLINK_RESULT_RUNTIME_STOPPED: yunlink_result_t = 11;
pub const YUNLINK_RESULT_PROTOCOL_MISMATCH: yunlink_result_t = 12;
pub const YUNLINK_RESULT_UNAUTHORIZED: yunlink_result_t = 13;
pub const YUNLINK_RESULT_REJECTED: yunlink_result_t = 14;
pub const YUNLINK_RESULT_INTERNAL: yunlink_result_t = 15;
pub const YUNLINK_RESULT_NOT_FOUND: yunlink_result_t = 100;

pub const YUNLINK_AGENT_TYPE_GROUND_STATION: u8 = 1;
pub const YUNLINK_AGENT_TYPE_UAV: u8 = 2;

pub const YUNLINK_ROLE_OBSERVER: u8 = 1;
pub const YUNLINK_ROLE_CONTROLLER: u8 = 2;
pub const YUNLINK_ROLE_VEHICLE: u8 = 3;

pub const YUNLINK_TARGET_SCOPE_ENTITY: u8 = 1;

pub const YUNLINK_CONTROL_SOURCE_GROUND_STATION: u8 = 1;

pub const YUNLINK_AUTHORITY_STATE_CONTROLLER: u8 = 2;

pub const YUNLINK_RUNTIME_EVENT_NONE: u8 = 0;
pub const YUNLINK_RUNTIME_EVENT_LINK: u8 = 1;
pub const YUNLINK_RUNTIME_EVENT_ERROR: u8 = 2;
pub const YUNLINK_RUNTIME_EVENT_COMMAND_RESULT: u8 = 3;
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE: u8 = 4;
pub const YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT: u8 = 5;
