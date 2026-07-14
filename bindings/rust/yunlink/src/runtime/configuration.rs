use yunlink_sys as sys;

use super::Runtime;
use crate::configuration::{string_view, ConfigFieldValue, ConfigurationHandle, NativePatch};
use crate::error::{ensure, Result};
use crate::types::{PeerConnection, Session, TargetSelector};

impl Runtime {
    /// Request the configuration resource list.
    pub fn configuration_resource_list(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<ConfigurationHandle> {
        let mut handle = sys::yunlink_configuration_handle_t::default();
        ensure(unsafe {
            sys::yunlink_configuration_publish_resource_list_request(
                self.raw_ptr(),
                &peer.raw,
                &session.to_native(),
                &target.raw,
                &mut handle,
            )
        })?;
        Ok(Self::configuration_handle_from_native(handle))
    }

    /// Request a provider-neutral field schema for one resource.
    pub fn configuration_resource_describe(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        resource_id: &str,
    ) -> Result<ConfigurationHandle> {
        let mut handle = sys::yunlink_configuration_handle_t::default();
        ensure(unsafe {
            sys::yunlink_configuration_publish_resource_describe_request(
                self.raw_ptr(),
                &peer.raw,
                &session.to_native(),
                &target.raw,
                string_view(resource_id),
                &mut handle,
            )
        })?;
        Ok(Self::configuration_handle_from_native(handle))
    }

    /// Request the complete current snapshot for one resource.
    pub fn configuration_resource_get(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        resource_id: &str,
    ) -> Result<ConfigurationHandle> {
        let mut handle = sys::yunlink_configuration_handle_t::default();
        ensure(unsafe {
            sys::yunlink_configuration_publish_resource_get_request(
                self.raw_ptr(),
                &peer.raw,
                &session.to_native(),
                &target.raw,
                string_view(resource_id),
                &mut handle,
            )
        })?;
        Ok(Self::configuration_handle_from_native(handle))
    }

    /// Validate or persist a field-level patch guarded by an expected revision.
    pub fn configuration_resource_patch(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        resource_id: &str,
        expected_revision: &str,
        updates: &[ConfigFieldValue],
        validate_only: bool,
    ) -> Result<ConfigurationHandle> {
        let native = NativePatch::new(updates);
        let mut handle = sys::yunlink_configuration_handle_t::default();
        ensure(unsafe {
            sys::yunlink_configuration_publish_resource_patch_request(
                self.raw_ptr(),
                &peer.raw,
                &session.to_native(),
                &target.raw,
                string_view(resource_id),
                string_view(expected_revision),
                native.updates.as_ptr(),
                native.updates.len(),
                u8::from(validate_only),
                &mut handle,
            )
        })?;
        Ok(Self::configuration_handle_from_native(handle))
    }

    /// Apply exactly the requested stored revision.
    pub fn configuration_resource_apply(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        resource_id: &str,
        expected_revision: &str,
    ) -> Result<ConfigurationHandle> {
        let mut handle = sys::yunlink_configuration_handle_t::default();
        ensure(unsafe {
            sys::yunlink_configuration_publish_resource_apply_request(
                self.raw_ptr(),
                &peer.raw,
                &session.to_native(),
                &target.raw,
                string_view(resource_id),
                string_view(expected_revision),
                &mut handle,
            )
        })?;
        Ok(Self::configuration_handle_from_native(handle))
    }

    fn configuration_handle_from_native(
        handle: sys::yunlink_configuration_handle_t,
    ) -> ConfigurationHandle {
        ConfigurationHandle {
            message_id: handle.message_id,
            session_id: handle.session_id,
            created_at_ms: handle.created_at_ms,
            ttl_ms: handle.ttl_ms,
        }
    }
}
