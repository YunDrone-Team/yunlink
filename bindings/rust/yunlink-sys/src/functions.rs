use std::ffi::c_char;

use crate::configuration::{
    yunlink_config_field_value_view_t, yunlink_config_resource_apply_response_callback_t,
    yunlink_config_resource_describe_response_callback_t,
    yunlink_config_resource_get_response_callback_t,
    yunlink_config_resource_list_response_callback_t,
    yunlink_config_resource_patch_response_callback_t, yunlink_configuration_handle_t,
    yunlink_string_view_t,
};
use crate::constants::yunlink_result_t;
use crate::events::yunlink_runtime_event_t;
use crate::managed_entities::{
    yunlink_managed_entity_attachment_response_callback_t,
    yunlink_managed_entity_directory_changed_callback_t,
    yunlink_managed_entity_list_response_callback_t,
};
use crate::runtime_logs::{
    yunlink_runtime_log_list_response_callback_t, yunlink_runtime_log_read_response_callback_t,
};
use crate::types::{
    yunlink_authority_lease_t, yunlink_command_handle_t, yunlink_goto_command_t,
    yunlink_land_command_t, yunlink_local_odom_t, yunlink_peer_t, yunlink_return_command_t,
    yunlink_runtime_config_t, yunlink_runtime_t, yunlink_session_info_t, yunlink_session_t,
    yunlink_takeoff_command_t, yunlink_target_selector_t, yunlink_uav_control_command_t,
    yunlink_ugv_control_command_t, yunlink_vehicle_core_state_t,
    yunlink_velocity_setpoint_command_t,
};

// Raw extern declarations for `libyunlink_ffi`.
//
// This module is intentionally a thin mirror of
// `include/yunlink/c/abi/functions.h`. It does not validate pointers, convert
// strings, manage ownership, or map result codes. The safe `yunlink` crate is
// the layer that performs those tasks.
unsafe extern "C" {
    /// Return the numeric C ABI version exported by the loaded library.
    pub fn yunlink_ffi_abi_version() -> u32;
    /// Return a static C string for a raw result code.
    pub fn yunlink_result_name(result: yunlink_result_t) -> *const c_char;

    /// Allocate a runtime owned by the C++ core.
    pub fn yunlink_runtime_create(out_runtime: *mut *mut yunlink_runtime_t) -> yunlink_result_t;
    /// Destroy a runtime previously returned by `yunlink_runtime_create`.
    pub fn yunlink_runtime_destroy(runtime: *mut yunlink_runtime_t);
    /// Start the runtime with a fully initialized config struct.
    pub fn yunlink_runtime_start(
        runtime: *mut yunlink_runtime_t,
        cfg: *const yunlink_runtime_config_t,
    ) -> yunlink_result_t;
    /// Stop the runtime and its transports.
    pub fn yunlink_runtime_stop(runtime: *mut yunlink_runtime_t) -> yunlink_result_t;

    /// Connect the runtime to a TCP peer and write the raw peer handle.
    pub fn yunlink_peer_connect(
        runtime: *mut yunlink_runtime_t,
        ip: *const c_char,
        port: u16,
        out_peer: *mut yunlink_peer_t,
    ) -> yunlink_result_t;
    /// Close the TCP transport associated with a peer handle.
    pub fn yunlink_peer_disconnect(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
    ) -> yunlink_result_t;
    /// Open a protocol session with an already connected peer.
    pub fn yunlink_session_open(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        node_name: *const c_char,
        out_session: *mut yunlink_session_t,
    ) -> yunlink_result_t;
    /// Describe a session known to the runtime.
    pub fn yunlink_session_describe(
        runtime: *mut yunlink_runtime_t,
        session: *const yunlink_session_t,
        out_info: *mut yunlink_session_info_t,
    ) -> yunlink_result_t;

    /// Request command authority for a target.
    pub fn yunlink_authority_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
        allow_preempt: u8,
    ) -> yunlink_result_t;
    /// Renew an existing authority lease.
    pub fn yunlink_authority_renew(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        source: u8,
        lease_ttl_ms: u32,
    ) -> yunlink_result_t;
    /// Release command authority for a target.
    pub fn yunlink_authority_release(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
    ) -> yunlink_result_t;
    /// Return the current authority lease observed by the runtime.
    pub fn yunlink_authority_current(
        runtime: *mut yunlink_runtime_t,
        out_lease: *mut yunlink_authority_lease_t,
    ) -> yunlink_result_t;

    /// Publish a takeoff command and write a correlation handle.
    pub fn yunlink_command_publish_takeoff(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_takeoff_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    /// Publish a land command and write a correlation handle.
    pub fn yunlink_command_publish_land(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_land_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    /// Publish a return-to-home command and write a correlation handle.
    pub fn yunlink_command_publish_return(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_return_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    /// Publish a goto command and write a correlation handle.
    pub fn yunlink_command_publish_goto(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_goto_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    /// Publish a velocity setpoint command and write a correlation handle.
    pub fn yunlink_command_publish_velocity_setpoint(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_velocity_setpoint_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_command_publish_uav_control(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_uav_control_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_command_publish_ugv_control(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_ugv_control_command_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;

    /// Publish a vehicle core state snapshot from a vehicle-like runtime.
    pub fn yunlink_publish_vehicle_core_state(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_vehicle_core_state_t,
        session_id: u64,
    ) -> yunlink_result_t;

    pub fn yunlink_system_service_request_feature_list(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_feature_get(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        feature_name: *const c_char,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_feature_start(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        feature_name: *const c_char,
        override_args: *const *const c_char,
        override_arg_count: usize,
        restart_if_running: u8,
        start_with_terminal: u8,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_runtime_log_list(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_runtime_log_read(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        runtime_id: yunlink_string_view_t,
        cursor: u64,
        max_bytes: u32,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_managed_entity_list(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_managed_entity_attachment(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        endpoint_uid: yunlink_string_view_t,
        directory_revision: yunlink_string_view_t,
        action: u8,
        entity_uids: *const yunlink_string_view_t,
        entity_uid_count: usize,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_topic_list(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_request_topic_subscription(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        topic_name: *const c_char,
        subscribe: u8,
        max_rate_hz: f32,
        max_payload_bytes: u32,
        out_handle: *mut yunlink_command_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_subscribe_runtime_log_list_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_runtime_log_list_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_subscribe_runtime_log_read_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_runtime_log_read_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_subscribe_managed_entity_list_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_managed_entity_list_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_subscribe_managed_entity_directory_changed(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_managed_entity_directory_changed_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_subscribe_managed_entity_attachment_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_managed_entity_attachment_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_system_service_unsubscribe(
        runtime: *mut yunlink_runtime_t,
        token: usize,
    ) -> yunlink_result_t;

    /// Poll one tagged runtime event.
    ///
    /// The ABI uses polling instead of callbacks so Rust does not need to expose
    /// callback trampolines, unwind behavior, or foreign thread ownership across
    /// this boundary.
    pub fn yunlink_runtime_poll_event(
        runtime: *mut yunlink_runtime_t,
        out_event: *mut yunlink_runtime_event_t,
    ) -> yunlink_result_t;

    pub fn yunlink_publish_local_odom(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        target: *const yunlink_target_selector_t,
        payload: *const yunlink_local_odom_t,
        session_id: u64,
    ) -> yunlink_result_t;

    pub fn yunlink_configuration_publish_resource_list_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        out_handle: *mut yunlink_configuration_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_publish_resource_describe_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        resource_id: yunlink_string_view_t,
        out_handle: *mut yunlink_configuration_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_publish_resource_get_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        resource_id: yunlink_string_view_t,
        out_handle: *mut yunlink_configuration_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_publish_resource_patch_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        resource_id: yunlink_string_view_t,
        expected_revision: yunlink_string_view_t,
        updates: *const yunlink_config_field_value_view_t,
        update_count: usize,
        validate_only: u8,
        out_handle: *mut yunlink_configuration_handle_t,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_publish_resource_apply_request(
        runtime: *mut yunlink_runtime_t,
        peer: *const yunlink_peer_t,
        session: *const yunlink_session_t,
        target: *const yunlink_target_selector_t,
        resource_id: yunlink_string_view_t,
        expected_revision: yunlink_string_view_t,
        out_handle: *mut yunlink_configuration_handle_t,
    ) -> yunlink_result_t;

    pub fn yunlink_configuration_subscribe_resource_list_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_config_resource_list_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_subscribe_resource_describe_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_config_resource_describe_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_subscribe_resource_get_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_config_resource_get_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_subscribe_resource_patch_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_config_resource_patch_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_subscribe_resource_apply_responses(
        runtime: *mut yunlink_runtime_t,
        callback: yunlink_config_resource_apply_response_callback_t,
        user_data: *mut core::ffi::c_void,
        out_token: *mut usize,
    ) -> yunlink_result_t;
    pub fn yunlink_configuration_unsubscribe(
        runtime: *mut yunlink_runtime_t,
        token: usize,
    ) -> yunlink_result_t;
}
