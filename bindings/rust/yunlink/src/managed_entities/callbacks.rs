use std::panic::{catch_unwind, AssertUnwindSafe};

use tokio::sync::broadcast;
use yunlink_sys as sys;

use crate::error::{ensure, Result};
use crate::types::{AgentType, EndpointIdentity, EndpointRole};

use super::{
    ManagedEntityAvailability, ManagedEntityDescriptor, ManagedEntityDirectory,
    ManagedEntityDirectoryChanged, ManagedEntityEvent,
};

unsafe fn string_from_view(view: sys::yunlink_string_view_t) -> Option<String> {
    if view.size == 0 {
        return Some(String::new());
    }
    if view.data.is_null() {
        return None;
    }
    let bytes = unsafe { std::slice::from_raw_parts(view.data.cast::<u8>(), view.size) };
    Some(String::from_utf8_lossy(bytes).into_owned())
}

unsafe fn slice_from_raw<'a, T>(pointer: *const T, count: usize) -> Option<&'a [T]> {
    if count == 0 {
        return Some(&[]);
    }
    if pointer.is_null() {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(pointer, count) })
}

unsafe fn identity_from_view(
    view: sys::yunlink_managed_entity_identity_view_t,
) -> Option<EndpointIdentity> {
    Some(EndpointIdentity {
        agent_type: AgentType::from_native(view.agent_type),
        agent_id: view.agent_id,
        role: EndpointRole::from_native(view.role),
        group_ids: unsafe { slice_from_raw(view.group_ids, view.group_id_count) }?.to_vec(),
    })
}

pub(crate) struct ManagedEntityCallbackContext {
    sender: broadcast::Sender<ManagedEntityEvent>,
}

fn publish_from_callback(
    user_data: *mut core::ffi::c_void,
    convert: impl FnOnce() -> Option<ManagedEntityEvent>,
) {
    if user_data.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let context = unsafe { &*(user_data.cast::<ManagedEntityCallbackContext>()) };
        if let Some(event) = convert() {
            let _ = context.sender.send(event);
        }
    }));
}

unsafe extern "C" fn on_directory(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_managed_entity_list_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        let entities = unsafe { slice_from_raw(view.entities, view.entity_count) }?
            .iter()
            .map(|entity| {
                let capabilities = unsafe {
                    slice_from_raw(entity.capabilities, entity.capability_count)
                }?
                .iter()
                .map(|value| unsafe { string_from_view(*value) })
                .collect::<Option<Vec<_>>>()?;
                Some(ManagedEntityDescriptor {
                    entity_uid: unsafe { string_from_view(entity.entity_uid) }?,
                    identity: unsafe { identity_from_view(entity.identity) }?,
                    display_name: unsafe { string_from_view(entity.display_name) }?,
                    hardware_id: unsafe { string_from_view(entity.hardware_id) }?,
                    capabilities,
                    availability: match entity.availability {
                        sys::YUNLINK_MANAGED_ENTITY_UNKNOWN => ManagedEntityAvailability::Unknown,
                        sys::YUNLINK_MANAGED_ENTITY_ONLINE => ManagedEntityAvailability::Online,
                        sys::YUNLINK_MANAGED_ENTITY_DEGRADED => ManagedEntityAvailability::Degraded,
                        sys::YUNLINK_MANAGED_ENTITY_OFFLINE => ManagedEntityAvailability::Offline,
                        other => ManagedEntityAvailability::Other(other),
                    },
                })
            })
            .collect::<Option<Vec<_>>>()?;
        Some(ManagedEntityEvent::Directory(ManagedEntityDirectory {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            success: view.success != 0,
            message: unsafe { string_from_view(view.message) }?,
            endpoint_uid: unsafe { string_from_view(view.endpoint_uid) }?,
            revision: unsafe { string_from_view(view.revision) }?,
            primary_identity: unsafe { identity_from_view(view.primary_identity) }?,
            entities,
        }))
    });
}

unsafe extern "C" fn on_changed(
    user_data: *mut core::ffi::c_void,
    event: *const sys::yunlink_managed_entity_directory_changed_view_t,
) {
    if event.is_null() {
        return;
    }
    let view = unsafe { *event };
    publish_from_callback(user_data, || {
        Some(ManagedEntityEvent::Changed(ManagedEntityDirectoryChanged {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            endpoint_uid: unsafe { string_from_view(view.endpoint_uid) }?,
            revision: unsafe { string_from_view(view.revision) }?,
        }))
    });
}

pub(crate) fn register_callbacks(
    runtime: *mut sys::yunlink_runtime_t,
    sender: broadcast::Sender<ManagedEntityEvent>,
) -> Result<(Box<ManagedEntityCallbackContext>, Vec<usize>)> {
    let mut context = Box::new(ManagedEntityCallbackContext { sender });
    let user_data = (&mut *context as *mut ManagedEntityCallbackContext).cast();
    let mut tokens = Vec::new();
    let mut token = 0;
    ensure(unsafe {
        sys::yunlink_system_service_subscribe_managed_entity_list_responses(
            runtime,
            Some(on_directory),
            user_data,
            &mut token,
        )
    })?;
    tokens.push(token);
    token = 0;
    if let Err(error) = ensure(unsafe {
        sys::yunlink_system_service_subscribe_managed_entity_directory_changed(
            runtime,
            Some(on_changed),
            user_data,
            &mut token,
        )
    }) {
        let _ = unsafe { sys::yunlink_system_service_unsubscribe(runtime, tokens[0]) };
        return Err(error);
    }
    tokens.push(token);
    Ok((context, tokens))
}

#[cfg(test)]
mod tests {
    use super::*;

    fn string_view(value: &str) -> sys::yunlink_string_view_t {
        sys::yunlink_string_view_t {
            data: value.as_ptr().cast(),
            size: value.len(),
        }
    }

    #[test]
    fn directory_callback_deep_copies_every_nested_view() {
        let (sender, mut receiver) = broadcast::channel(1);
        let mut context = ManagedEntityCallbackContext { sender };
        let mut endpoint_uid = String::from("endpoint-owned");
        let mut revision = String::from("revision-owned");
        let mut entity_uid = String::from("entity-owned");
        let mut display_name = String::from("UAV owned");
        let mut hardware_id = String::from("SIM-owned");
        let mut capability = String::from("telemetry.px4");
        let group_ids = vec![7_u32, 9_u32];
        let capability_views = vec![string_view(&capability)];
        let identity = sys::yunlink_managed_entity_identity_view_t {
            agent_type: sys::YUNLINK_AGENT_TYPE_UAV,
            agent_id: 2,
            role: sys::YUNLINK_ROLE_VEHICLE,
            group_ids: group_ids.as_ptr(),
            group_id_count: group_ids.len(),
        };
        let entities = vec![sys::yunlink_managed_entity_descriptor_view_t {
            entity_uid: string_view(&entity_uid),
            identity,
            display_name: string_view(&display_name),
            hardware_id: string_view(&hardware_id),
            capabilities: capability_views.as_ptr(),
            capability_count: capability_views.len(),
            availability: sys::YUNLINK_MANAGED_ENTITY_ONLINE,
        }];
        let response = sys::yunlink_managed_entity_list_response_view_t {
            session_id: 10,
            message_id: 20,
            correlation_id: 19,
            success: 1,
            message: string_view("ok"),
            endpoint_uid: string_view(&endpoint_uid),
            revision: string_view(&revision),
            primary_identity: identity,
            entities: entities.as_ptr(),
            entity_count: entities.len(),
        };

        unsafe {
            on_directory(
                (&mut context as *mut ManagedEntityCallbackContext).cast(),
                &response,
            )
        };
        endpoint_uid.replace_range(.., "mutated");
        revision.replace_range(.., "mutated");
        entity_uid.replace_range(.., "mutated");
        display_name.replace_range(.., "mutated");
        hardware_id.replace_range(.., "mutated");
        capability.replace_range(.., "mutated");

        let event = receiver.try_recv().expect("owned callback event");
        let ManagedEntityEvent::Directory(directory) = event else {
            panic!("expected directory event");
        };
        assert_eq!(directory.endpoint_uid, "endpoint-owned");
        assert_eq!(directory.revision, "revision-owned");
        assert_eq!(directory.entities[0].entity_uid, "entity-owned");
        assert_eq!(directory.entities[0].display_name, "UAV owned");
        assert_eq!(directory.entities[0].hardware_id, "SIM-owned");
        assert_eq!(directory.entities[0].capabilities, ["telemetry.px4"]);
        assert_eq!(directory.entities[0].identity.group_ids, [7, 9]);
    }
}
