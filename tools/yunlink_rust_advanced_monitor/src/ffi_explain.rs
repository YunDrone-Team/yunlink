//! Static teaching data for the ABI page.
//!
//! This module is the only monitor module that imports `yunlink-sys`. It does so
//! only to read ABI metadata and to name the raw symbols in the UI. Runtime
//! behavior still flows through the safe `yunlink` crate.

use yunlink_sys as sys;

/// One row in the safe-Rust-to-C-ABI mapping table.
#[derive(Debug, Clone)]
pub struct AbiMapping {
    /// Public safe Rust method used by app/runtime-client code.
    pub safe_api: &'static str,
    /// Raw Rust extern symbol declared by `yunlink-sys`.
    pub sys_symbol: &'static str,
    /// Exported C ABI function name in `libyunlink_ffi`.
    pub c_abi: &'static str,
    /// C ABI structs that carry data across the language boundary.
    pub c_structs: &'static str,
}

/// Return the loaded library ABI version through the raw sys layer.
///
/// This is safe to expose in the teaching page because the function has no
/// parameters and returns a plain integer ABI version.
pub fn abi_version() -> u32 {
    unsafe { sys::yunlink_ffi_abi_version() }
}

/// Static mapping between monitor-facing safe APIs and their C ABI symbols.
pub fn mappings() -> &'static [AbiMapping] {
    &[
        AbiMapping {
            safe_api: "Runtime::start",
            sys_symbol: "yunlink_sys::yunlink_runtime_start",
            c_abi: "yunlink_runtime_start",
            c_structs: "yunlink_runtime_config_t",
        },
        AbiMapping {
            safe_api: "Runtime::connect",
            sys_symbol: "yunlink_sys::yunlink_peer_connect",
            c_abi: "yunlink_peer_connect",
            c_structs: "yunlink_peer_t",
        },
        AbiMapping {
            safe_api: "Runtime::open_session",
            sys_symbol: "yunlink_sys::yunlink_session_open",
            c_abi: "yunlink_session_open",
            c_structs: "yunlink_session_t",
        },
        AbiMapping {
            safe_api: "Runtime::request_authority",
            sys_symbol: "yunlink_sys::yunlink_authority_request",
            c_abi: "yunlink_authority_request",
            c_structs: "yunlink_target_selector_t",
        },
        AbiMapping {
            safe_api: "Runtime::publish_goto",
            sys_symbol: "yunlink_sys::yunlink_command_publish_goto",
            c_abi: "yunlink_command_publish_goto",
            c_structs: "yunlink_goto_command_t",
        },
        AbiMapping {
            safe_api: "Runtime::publish_velocity_setpoint",
            sys_symbol: "yunlink_sys::yunlink_command_publish_velocity_setpoint",
            c_abi: "yunlink_command_publish_velocity_setpoint",
            c_structs: "yunlink_velocity_setpoint_command_t",
        },
        AbiMapping {
            safe_api: "Runtime::subscribe",
            sys_symbol: "yunlink_sys::yunlink_runtime_poll_event",
            c_abi: "yunlink_runtime_poll_event",
            c_structs: "yunlink_runtime_event_t",
        },
    ]
}

/// Static explanations for the C structs most relevant to the prototype.
pub fn struct_examples() -> &'static [(&'static str, &'static str)] {
    &[
        (
            "yunlink_runtime_config_t",
            "struct_size, UDP/TCP ports, identity, capability_flags, fixed shared_secret buffer",
        ),
        (
            "yunlink_target_selector_t",
            "struct_size, scope, target_type, entity_id, group_id",
        ),
        (
            "yunlink_command_handle_t",
            "session_id, message_id, correlation_id, target",
        ),
        (
            "yunlink_runtime_event_t",
            "type tag plus C union for link/error/command_result/state events",
        ),
    ]
}

#[cfg(test)]
mod tests {
    use super::mappings;

    #[test]
    fn abi_mapping_explains_the_runtime_path() {
        let symbols: Vec<&str> = mappings().iter().map(|item| item.c_abi).collect();

        assert!(symbols.contains(&"yunlink_runtime_start"));
        assert!(symbols.contains(&"yunlink_command_publish_goto"));
        assert!(symbols.contains(&"yunlink_runtime_poll_event"));
    }
}
