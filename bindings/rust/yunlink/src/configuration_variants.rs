//! Named-variant request and response payloads for Configuration Service.

use crate::configuration::{
    ConfigApplyOutcome, ConfigEffects, ConfigResourceDescriptor, ConfigServiceStatus,
    ConfigVariantDescriptor, ConfigVariantSource,
};

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantListRequest {
    pub resource_id: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantListResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub active_variant_id: String,
    pub variants: Vec<ConfigVariantDescriptor>,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantCreateRequest {
    pub resource_id: String,
    pub variant_id: String,
    pub source: ConfigVariantSource,
    pub expected_active_revision: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantCreateResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub variant: ConfigVariantDescriptor,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantSaveCurrentRequest {
    pub resource_id: String,
    pub variant_id: String,
    pub expected_variant_revision: String,
    pub expected_active_revision: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantSaveCurrentResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub variant: ConfigVariantDescriptor,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantActivateRequest {
    pub resource_id: String,
    pub variant_id: String,
    pub expected_active_revision: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantActivateResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
    pub applied_revision: String,
    pub outcome: ConfigApplyOutcome,
    pub effects: ConfigEffects,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantDeleteRequest {
    pub resource_id: String,
    pub variant_id: String,
    pub expected_revision: String,
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ConfigResourceVariantDeleteResponse {
    pub status: ConfigServiceStatus,
    pub message: String,
}

/// A resource descriptor helper useful to SDK callers choosing whether variants are available.
pub fn supports_variants(resource: &ConfigResourceDescriptor) -> bool {
    resource.variants_supported
}
