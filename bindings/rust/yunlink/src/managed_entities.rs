//! Owned managed-entity directory types and borrowed C callback conversion.

mod callbacks;

use crate::types::EndpointIdentity;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ManagedEntityAvailability {
    Unknown,
    Online,
    Degraded,
    Offline,
    Other(u8),
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManagedEntityDescriptor {
    pub entity_uid: String,
    pub identity: EndpointIdentity,
    pub display_name: String,
    pub hardware_id: String,
    pub capabilities: Vec<String>,
    pub availability: ManagedEntityAvailability,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManagedEntityDirectory {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub endpoint_uid: String,
    pub revision: String,
    pub primary_identity: EndpointIdentity,
    pub entities: Vec<ManagedEntityDescriptor>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManagedEntityDirectoryChanged {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub endpoint_uid: String,
    pub revision: String,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ManagedEntityAttachmentAction {
    Attach,
    Detach,
}

impl ManagedEntityAttachmentAction {
    pub(crate) const fn as_native(self) -> u8 {
        match self {
            Self::Attach => 1,
            Self::Detach => 2,
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManagedEntityAttachmentResult {
    pub entity_uid: String,
    pub accepted: bool,
    pub message: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ManagedEntityAttachmentResponse {
    pub session_id: u64,
    pub message_id: u64,
    pub correlation_id: u64,
    pub success: bool,
    pub message: String,
    pub endpoint_uid: String,
    pub directory_revision: String,
    pub results: Vec<ManagedEntityAttachmentResult>,
    pub attached_entity_uids: Vec<String>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ManagedEntityEvent {
    Directory(ManagedEntityDirectory),
    Changed(ManagedEntityDirectoryChanged),
    Attachment(ManagedEntityAttachmentResponse),
}

pub(crate) use callbacks::{register_callbacks, ManagedEntityCallbackContext};
