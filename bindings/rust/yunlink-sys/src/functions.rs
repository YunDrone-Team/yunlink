use std::ffi::c_char;

use crate::constants::yunlink_result_t;
use crate::events::yunlink_runtime_event_t;
use crate::types::{
    yunlink_authority_lease_t, yunlink_command_handle_t, yunlink_goto_command_t,
    yunlink_peer_t, yunlink_runtime_config_t, yunlink_runtime_t, yunlink_session_t,
    yunlink_target_selector_t, yunlink_vehicle_core_state_t,
};

unsafe extern "C" {
    pub fn yunlink_ffi_abi_version() -> u32;
    pub fn yunlink_result_name(result: yunlink_result_t) -> *const c_char;

    pub fn yunlink_runtime_create(out_runtime: *mut *mut yunlink_runtime_t) -> yunlink_result_t;
    pub fn yunlink_runtime_destroy(runtime: *mut yunlink_runtime_t);
    pub fn yunlink_runtime_start(
        runtime: *mut yunlink_runtime_t,
        cfg: *const yunlink_runtime_config_t,
    ) -> yunlink_result_t;
    pub fn yunlink_runtime_stop(runtime: *mut yunlink_runtime_t) -> yunlink_result_t;

    pub fn yunlink_peer_connect(
        runtime: *mut yunlink_runtime_t,
        ip: *const c_char,
        port: u16,
        out_peer: *mut yunlink_peer_t,
    ) -> yunlink_result_t;
    pub fn yunlink_session_open(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        node_name: *const c_char,
        out_session: *mut yunlink_session_t,
    ) -> yunlink_result_t;

    pub fn yunlink_authority_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
        allow_preempt: u8,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_renew(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_release(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
    ) -> yunlink_result_t;
    pub fn yunlink_authority_current(
        runtime: *mut yunlink_runtime_t,
        out_lease: *mut yunlink_authority_lease_t,
    ) -> yunlink_result_t;

    pub fn yunlink_command_publish_goto(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_goto_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;

    pub fn yunlink_publish_vehicle_core_state(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_vehicle_core_state_t,
        session_id: u64,
    ) -> yunlink_result_t;

    pub fn yunlink_runtime_poll_event(
        runtime: *mut yunlink_runtime_t,
        out_event: *mut yunlink_runtime_event_t,
    ) -> yunlink_result_t;
}
