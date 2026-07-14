use yunlink_sys as sys;

#[derive(Debug, Clone, PartialEq)]
pub enum ConfigValue {
    Bool(bool),
    Int64(i64),
    Double(f64),
    String(String),
    StringList(Vec<String>),
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfigStatus {
    Ok,
    NotFound,
    Unsupported,
    Unauthenticated,
    Unauthorized,
    Conflict,
    Invalid,
    UnsafeState,
    InternalError,
    Other(u8),
}

impl ConfigStatus {
    pub(super) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_CONFIG_STATUS_OK => Self::Ok,
            sys::YUNLINK_CONFIG_STATUS_NOT_FOUND => Self::NotFound,
            sys::YUNLINK_CONFIG_STATUS_UNSUPPORTED => Self::Unsupported,
            sys::YUNLINK_CONFIG_STATUS_UNAUTHENTICATED => Self::Unauthenticated,
            sys::YUNLINK_CONFIG_STATUS_UNAUTHORIZED => Self::Unauthorized,
            sys::YUNLINK_CONFIG_STATUS_CONFLICT => Self::Conflict,
            sys::YUNLINK_CONFIG_STATUS_INVALID => Self::Invalid,
            sys::YUNLINK_CONFIG_STATUS_UNSAFE_STATE => Self::UnsafeState,
            sys::YUNLINK_CONFIG_STATUS_INTERNAL_ERROR => Self::InternalError,
            other => Self::Other(other),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfigApplyRequirement {
    None,
    ComponentRestart,
    EndpointRestart,
    DeviceReboot,
    Manual,
    Other(u8),
}

impl ConfigApplyRequirement {
    pub(super) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_CONFIG_APPLY_NONE => Self::None,
            sys::YUNLINK_CONFIG_APPLY_COMPONENT_RESTART => Self::ComponentRestart,
            sys::YUNLINK_CONFIG_APPLY_ENDPOINT_RESTART => Self::EndpointRestart,
            sys::YUNLINK_CONFIG_APPLY_DEVICE_REBOOT => Self::DeviceReboot,
            sys::YUNLINK_CONFIG_APPLY_MANUAL => Self::Manual,
            other => Self::Other(other),
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ConfigApplyOutcome {
    Applied,
    RestartScheduled,
    ManualActionRequired,
    Failed,
    Other(u8),
}

impl ConfigApplyOutcome {
    pub(super) fn from_native(value: u8) -> Self {
        match value {
            sys::YUNLINK_CONFIG_OUTCOME_APPLIED => Self::Applied,
            sys::YUNLINK_CONFIG_OUTCOME_RESTART_SCHEDULED => Self::RestartScheduled,
            sys::YUNLINK_CONFIG_OUTCOME_MANUAL_ACTION_REQUIRED => Self::ManualActionRequired,
            sys::YUNLINK_CONFIG_OUTCOME_FAILED => Self::Failed,
            other => Self::Other(other),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceDescriptor {
    pub id: String,
    pub title: String,
    pub description: String,
    pub readable: bool,
    pub writable: bool,
    pub apply_supported: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigChoice {
    pub value: ConfigValue,
    pub label: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigFieldSchema {
    pub path: String,
    pub title: String,
    pub description: String,
    pub value_type: u8,
    pub required: bool,
    pub read_only: bool,
    pub sensitive: bool,
    pub minimum: Option<f64>,
    pub maximum: Option<f64>,
    pub validation_pattern: String,
    pub choices: Vec<ConfigChoice>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigFieldValue {
    pub path: String,
    pub value: ConfigValue,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigSnapshot {
    pub resource_id: String,
    pub revision: String,
    pub applied_revision: String,
    pub values: Vec<ConfigFieldValue>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigFieldError {
    pub path: String,
    pub code: String,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigEffects {
    pub requirement: ConfigApplyRequirement,
    pub affected_components: Vec<String>,
    pub reconnect_expected: bool,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct ConfigurationHandle {
    pub message_id: u64,
    pub session_id: u64,
    pub created_at_ms: u64,
    pub ttl_ms: u32,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceListResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub status: ConfigStatus,
    pub message: String,
    pub resources: Vec<ConfigResourceDescriptor>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceDescribeResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub status: ConfigStatus,
    pub message: String,
    pub resource: ConfigResourceDescriptor,
    pub fields: Vec<ConfigFieldSchema>,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceGetResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub status: ConfigStatus,
    pub message: String,
    pub snapshot: ConfigSnapshot,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourcePatchResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub status: ConfigStatus,
    pub message: String,
    pub snapshot: ConfigSnapshot,
    pub errors: Vec<ConfigFieldError>,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceApplyResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub status: ConfigStatus,
    pub message: String,
    pub applied_revision: String,
    pub outcome: ConfigApplyOutcome,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ConfigurationResponse {
    List(ConfigResourceListResponse),
    Describe(ConfigResourceDescribeResponse),
    Get(ConfigResourceGetResponse),
    Patch(ConfigResourcePatchResponse),
    Apply(ConfigResourceApplyResponse),
}
