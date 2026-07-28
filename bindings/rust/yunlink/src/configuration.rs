//! Provider-neutral typed Configuration Service payload models.

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigValueType {
    Bool = 1,
    Int64 = 2,
    Double = 3,
    String = 4,
    StringList = 5,
    DoubleList = 6,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigServiceStatus {
    Ok = 0,
    NotFound = 1,
    Unsupported = 2,
    Unauthenticated = 3,
    Unauthorized = 4,
    Conflict = 5,
    Invalid = 6,
    UnsafeState = 7,
    InternalError = 8,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigApplyRequirement {
    None = 0,
    ComponentRestart = 1,
    EndpointRestart = 2,
    DeviceReboot = 3,
    Manual = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigApplyOutcome {
    Applied = 1,
    RestartScheduled = 2,
    ManualActionRequired = 3,
    Failed = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigFieldUpdatePolicy {
    HotReload = 0,
    ComponentRestart = 1,
    EndpointRestart = 2,
    DeviceReboot = 3,
    Manual = 4,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum ConfigVariantSource {
    Default = 1,
    Active = 2,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ConfigValue {
    Bool(bool),
    Int64(i64),
    Double(f64),
    String(String),
    StringList(Vec<String>),
    DoubleList(Vec<f64>),
}

impl ConfigValue {
    pub fn value_type(&self) -> ConfigValueType {
        match self {
            Self::Bool(_) => ConfigValueType::Bool,
            Self::Int64(_) => ConfigValueType::Int64,
            Self::Double(_) => ConfigValueType::Double,
            Self::String(_) => ConfigValueType::String,
            Self::StringList(_) => ConfigValueType::StringList,
            Self::DoubleList(_) => ConfigValueType::DoubleList,
        }
    }
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceDescriptor {
    pub id: String,
    pub title: String,
    pub description: String,
    pub readable: bool,
    pub writable: bool,
    pub apply_supported: bool,
    pub variants_supported: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigChoice {
    pub value: ConfigValue,
    pub label: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigFieldSchema {
    pub path: String,
    pub group_path: String,
    pub title: String,
    pub description: String,
    pub value_type: ConfigValueType,
    pub required: bool,
    pub read_only: bool,
    pub sensitive: bool,
    pub minimum: Option<f64>,
    pub maximum: Option<f64>,
    pub validation_pattern: String,
    pub choices: Vec<ConfigChoice>,
    pub update_policy: ConfigFieldUpdatePolicy,
    pub unit: String,
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
    pub variant_id: String,
    pub active_variant_id: String,
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

impl Default for ConfigEffects {
    fn default() -> Self {
        Self {
            requirement: ConfigApplyRequirement::None,
            affected_components: Vec::new(),
            reconnect_expected: false,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigVariantDescriptor {
    pub id: String,
    pub title: String,
    pub revision: String,
    pub modified_at_ns: u64,
    pub active: bool,
    pub mutable_variant: bool,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceListRequest;

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceListResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub resources: Vec<ConfigResourceDescriptor>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceDescribeRequest {
    pub resource_id: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceDescribeResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub resource: ConfigResourceDescriptor,
    pub fields: Vec<ConfigFieldSchema>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceGetRequest {
    pub resource_id: String,
    pub variant_id: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceGetResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub snapshot: ConfigSnapshot,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourcePatchRequest {
    pub resource_id: String,
    pub variant_id: String,
    pub expected_revision: String,
    pub updates: Vec<ConfigFieldValue>,
    pub validate_only: bool,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourcePatchResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    /// Persisted base snapshot. For a dry-run its revision is the revision that a
    /// subsequent save must use together with the same updates.
    pub snapshot: ConfigSnapshot,
    /// Provider-normalized preview for a successful dry-run. Its revision identifies
    /// candidate content only; it is not an expected_revision token for a save.
    pub candidate_snapshot: Option<ConfigSnapshot>,
    pub errors: Vec<ConfigFieldError>,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceApplyRequest {
    pub resource_id: String,
    pub expected_revision: String,
}

#[derive(Debug, Clone, PartialEq)]
pub struct ConfigResourceApplyResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub applied_revision: String,
    pub outcome: ConfigApplyOutcome,
    pub effects: ConfigEffects,
}
