use std::ffi::CString;

use yunlink_sys as sys;

use crate::error::{ensure, Result};
use crate::runtime::Runtime;
use crate::runtime_logs::string_view;
use crate::types::{CommandHandle, PeerConnection, Session, TargetSelector};

impl Runtime {
    /// Request the current ROS topic catalogue from the remote runtime.
    pub async fn request_topic_list(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_topic_list(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    /// Explicitly subscribe or unsubscribe one bounded ROS topic stream.
    pub async fn request_topic_subscription(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        topic_name: &str,
        subscribe: bool,
        max_rate_hz: f32,
        max_payload_bytes: u32,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let topic_name = CString::new(topic_name)?;
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_topic_subscription(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                topic_name.as_ptr(),
                u8::from(subscribe),
                max_rate_hz,
                max_payload_bytes,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    /// Request the authenticated logical-entity directory for an endpoint session.
    pub async fn request_managed_entity_list(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_managed_entity_list(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    pub async fn request_feature_list(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_feature_list(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    pub async fn request_feature_get(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        feature_name: &str,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let feature_name = CString::new(feature_name)?;
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_feature_get(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                feature_name.as_ptr(),
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    pub async fn request_feature_start(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        feature_name: &str,
        override_args: &[String],
        restart_if_running: bool,
        start_with_terminal: bool,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let feature_name = CString::new(feature_name)?;
        let override_args = override_args
            .iter()
            .map(|value| CString::new(value.as_str()))
            .collect::<std::result::Result<Vec<_>, _>>()?;
        let override_arg_ptrs = override_args
            .iter()
            .map(|value| value.as_ptr())
            .collect::<Vec<_>>();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_feature_start(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                feature_name.as_ptr(),
                override_arg_ptrs.as_ptr(),
                override_arg_ptrs.len(),
                u8::from(restart_if_running),
                u8::from(start_with_terminal),
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    /// Request the device-managed runtime log catalogue.
    pub async fn request_runtime_log_list(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_runtime_log_list(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }

    /// Read a bounded chunk from a device-managed runtime log.
    pub async fn request_runtime_log_read(
        &self,
        peer: &PeerConnection,
        session: &Session,
        target: &TargetSelector,
        runtime_id: &str,
        cursor: u64,
        max_bytes: u32,
    ) -> Result<CommandHandle> {
        let session = session.to_native();
        let mut handle = sys::yunlink_command_handle_t::default();
        ensure(unsafe {
            sys::yunlink_system_service_request_runtime_log_read(
                self.raw_ptr(),
                &peer.raw,
                &session,
                &target.raw,
                string_view(runtime_id),
                cursor,
                max_bytes,
                &mut handle,
            )
        })?;
        Ok(CommandHandle::from_raw(handle))
    }
}
