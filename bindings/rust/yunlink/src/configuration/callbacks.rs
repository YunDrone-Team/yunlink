use std::panic::{catch_unwind, AssertUnwindSafe};

use tokio::sync::broadcast;
use yunlink_sys as sys;

use crate::error::{ensure, Result};

use super::views::{
    descriptor_from_view, effects_from_view, slice_from_raw, snapshot_from_view, string_from_view,
    value_from_view,
};
use super::{
    ConfigApplyOutcome, ConfigChoice, ConfigFieldError, ConfigFieldSchema,
    ConfigResourceApplyResponse, ConfigResourceDescribeResponse, ConfigResourceGetResponse,
    ConfigResourceListResponse, ConfigResourcePatchResponse, ConfigStatus, ConfigurationResponse,
};

pub(crate) struct ConfigurationCallbackContext {
    sender: broadcast::Sender<ConfigurationResponse>,
}

fn publish_from_callback(
    user_data: *mut core::ffi::c_void,
    convert: impl FnOnce() -> Option<ConfigurationResponse>,
) {
    if user_data.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let context = unsafe { &*(user_data.cast::<ConfigurationCallbackContext>()) };
        if let Some(response) = convert() {
            let _ = context.sender.send(response);
        }
    }));
}

unsafe extern "C" fn on_list(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_config_resource_list_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        let resources = unsafe { slice_from_raw(view.resources, view.resource_count) }?
            .iter()
            .map(|item| unsafe { descriptor_from_view(*item) })
            .collect::<Option<Vec<_>>>()?;
        Some(ConfigurationResponse::List(ConfigResourceListResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            status: ConfigStatus::from_native(view.status),
            message: unsafe { string_from_view(view.message) }?,
            resources,
        }))
    });
}

unsafe extern "C" fn on_describe(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_config_resource_describe_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        let fields = unsafe { slice_from_raw(view.fields, view.field_count) }?
            .iter()
            .map(|item| {
                let choices = unsafe { slice_from_raw(item.choices, item.choice_count) }?
                    .iter()
                    .map(|choice| {
                        Some(ConfigChoice {
                            value: unsafe { value_from_view(choice.value) }?,
                            label: unsafe { string_from_view(choice.label) }?,
                        })
                    })
                    .collect::<Option<Vec<_>>>()?;
                Some(ConfigFieldSchema {
                    path: unsafe { string_from_view(item.path) }?,
                    title: unsafe { string_from_view(item.title) }?,
                    description: unsafe { string_from_view(item.description) }?,
                    value_type: item.type_,
                    required: item.required != 0,
                    read_only: item.read_only != 0,
                    sensitive: item.sensitive != 0,
                    minimum: (item.has_minimum != 0).then_some(item.minimum),
                    maximum: (item.has_maximum != 0).then_some(item.maximum),
                    validation_pattern: unsafe { string_from_view(item.validation_pattern) }?,
                    choices,
                })
            })
            .collect::<Option<Vec<_>>>()?;
        Some(ConfigurationResponse::Describe(
            ConfigResourceDescribeResponse {
                session_id: view.session_id,
                message_id: view.message_id,
                correlation_id: view.correlation_id,
                status: ConfigStatus::from_native(view.status),
                message: unsafe { string_from_view(view.message) }?,
                resource: unsafe { descriptor_from_view(view.resource) }?,
                fields,
            },
        ))
    });
}

unsafe extern "C" fn on_get(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_config_resource_get_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        Some(ConfigurationResponse::Get(ConfigResourceGetResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            status: ConfigStatus::from_native(view.status),
            message: unsafe { string_from_view(view.message) }?,
            snapshot: unsafe { snapshot_from_view(view.snapshot) }?,
        }))
    });
}

unsafe extern "C" fn on_patch(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_config_resource_patch_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        let errors = unsafe { slice_from_raw(view.errors, view.error_count) }?
            .iter()
            .map(|item| {
                Some(ConfigFieldError {
                    path: unsafe { string_from_view(item.path) }?,
                    code: unsafe { string_from_view(item.code) }?,
                    message: unsafe { string_from_view(item.message) }?,
                })
            })
            .collect::<Option<Vec<_>>>()?;
        Some(ConfigurationResponse::Patch(ConfigResourcePatchResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            status: ConfigStatus::from_native(view.status),
            message: unsafe { string_from_view(view.message) }?,
            snapshot: unsafe { snapshot_from_view(view.snapshot) }?,
            errors,
            effects: unsafe { effects_from_view(view.effects) }?,
        }))
    });
}

unsafe extern "C" fn on_apply(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_config_resource_apply_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        Some(ConfigurationResponse::Apply(ConfigResourceApplyResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            status: ConfigStatus::from_native(view.status),
            message: unsafe { string_from_view(view.message) }?,
            applied_revision: unsafe { string_from_view(view.applied_revision) }?,
            outcome: ConfigApplyOutcome::from_native(view.outcome),
            effects: unsafe { effects_from_view(view.effects) }?,
        }))
    });
}

pub(crate) fn register_callbacks(
    runtime: *mut sys::yunlink_runtime_t,
    sender: broadcast::Sender<ConfigurationResponse>,
) -> Result<(Box<ConfigurationCallbackContext>, Vec<usize>)> {
    let mut context = Box::new(ConfigurationCallbackContext { sender });
    let user_data = (&mut *context as *mut ConfigurationCallbackContext).cast();
    let mut tokens = Vec::new();
    let registrations: [unsafe fn(
        *mut sys::yunlink_runtime_t,
        *mut core::ffi::c_void,
        *mut usize,
    ) -> i32; 5] = [
        |runtime, user_data, token| unsafe {
            sys::yunlink_configuration_subscribe_resource_list_responses(
                runtime,
                Some(on_list),
                user_data,
                token,
            )
        },
        |runtime, user_data, token| unsafe {
            sys::yunlink_configuration_subscribe_resource_describe_responses(
                runtime,
                Some(on_describe),
                user_data,
                token,
            )
        },
        |runtime, user_data, token| unsafe {
            sys::yunlink_configuration_subscribe_resource_get_responses(
                runtime,
                Some(on_get),
                user_data,
                token,
            )
        },
        |runtime, user_data, token| unsafe {
            sys::yunlink_configuration_subscribe_resource_patch_responses(
                runtime,
                Some(on_patch),
                user_data,
                token,
            )
        },
        |runtime, user_data, token| unsafe {
            sys::yunlink_configuration_subscribe_resource_apply_responses(
                runtime,
                Some(on_apply),
                user_data,
                token,
            )
        },
    ];
    for register in registrations {
        let mut token = 0;
        if let Err(error) = ensure(unsafe { register(runtime, user_data, &mut token) }) {
            for registered in tokens {
                let _ = unsafe { sys::yunlink_configuration_unsubscribe(runtime, registered) };
            }
            return Err(error);
        }
        tokens.push(token);
    }
    Ok((context, tokens))
}
