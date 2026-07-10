use std::ffi::CString;

use yunlink_sys as sys;

use crate::error::{ensure, Result};
use crate::runtime::Runtime;
use crate::types::{CommandHandle, PeerConnection, Session, TargetSelector};

impl Runtime {
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
}
