use std::panic::{catch_unwind, AssertUnwindSafe};

use tokio::sync::broadcast;
use yunlink_sys as sys;

use crate::error::{ensure, Result};

use super::{
    RuntimeLogListResponse, RuntimeLogReadResponse, RuntimeLogResponse, RuntimeLogSummary,
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

pub(crate) struct RuntimeLogCallbackContext {
    sender: broadcast::Sender<RuntimeLogResponse>,
}

fn publish_from_callback(
    user_data: *mut core::ffi::c_void,
    convert: impl FnOnce() -> Option<RuntimeLogResponse>,
) {
    if user_data.is_null() {
        return;
    }
    let _ = catch_unwind(AssertUnwindSafe(|| {
        let context = unsafe { &*(user_data.cast::<RuntimeLogCallbackContext>()) };
        if let Some(response) = convert() {
            let _ = context.sender.send(response);
        }
    }));
}

unsafe extern "C" fn on_list(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_runtime_log_list_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        let runtimes = unsafe { slice_from_raw(view.runtimes, view.runtime_count) }?
            .iter()
            .map(|item| {
                Some(RuntimeLogSummary {
                    runtime_id: unsafe { string_from_view(item.runtime_id) }?,
                    feature_name: unsafe { string_from_view(item.feature_name) }?,
                    title: unsafe { string_from_view(item.title) }?,
                    state: unsafe { string_from_view(item.state) }?,
                    started_at_ns: item.started_at_ns,
                    finished_at_ns: item.finished_at_ns,
                    exit_code: (item.has_exit_code != 0).then_some(item.exit_code),
                    message: unsafe { string_from_view(item.message) }?,
                })
            })
            .collect::<Option<Vec<_>>>()?;
        Some(RuntimeLogResponse::List(RuntimeLogListResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            success: view.success != 0,
            message: unsafe { string_from_view(view.message) }?,
            runtimes,
        }))
    });
}

unsafe extern "C" fn on_read(
    user_data: *mut core::ffi::c_void,
    response: *const sys::yunlink_runtime_log_read_response_view_t,
) {
    if response.is_null() {
        return;
    }
    let view = unsafe { *response };
    publish_from_callback(user_data, || {
        Some(RuntimeLogResponse::Read(RuntimeLogReadResponse {
            session_id: view.session_id,
            message_id: view.message_id,
            correlation_id: view.correlation_id,
            success: view.success != 0,
            message: unsafe { string_from_view(view.message) }?,
            runtime_id: unsafe { string_from_view(view.runtime_id) }?,
            chunk: unsafe { string_from_view(view.chunk) }?,
            next_cursor: view.next_cursor,
            truncated: view.truncated != 0,
            eof: view.eof != 0,
        }))
    });
}

pub(crate) fn register_callbacks(
    runtime: *mut sys::yunlink_runtime_t,
    sender: broadcast::Sender<RuntimeLogResponse>,
) -> Result<(Box<RuntimeLogCallbackContext>, Vec<usize>)> {
    let mut context = Box::new(RuntimeLogCallbackContext { sender });
    let user_data = (&mut *context as *mut RuntimeLogCallbackContext).cast();
    let mut tokens = Vec::new();
    let registrations: [unsafe fn(
        *mut sys::yunlink_runtime_t,
        *mut core::ffi::c_void,
        *mut usize,
    ) -> i32; 2] = [
        |runtime, user_data, token| unsafe {
            sys::yunlink_system_service_subscribe_runtime_log_list_responses(
                runtime,
                Some(on_list),
                user_data,
                token,
            )
        },
        |runtime, user_data, token| unsafe {
            sys::yunlink_system_service_subscribe_runtime_log_read_responses(
                runtime,
                Some(on_read),
                user_data,
                token,
            )
        },
    ];
    for register in registrations {
        let mut token = 0;
        if let Err(error) = ensure(unsafe { register(runtime, user_data, &mut token) }) {
            for registered in tokens {
                let _ = unsafe { sys::yunlink_system_service_unsubscribe(runtime, registered) };
            }
            return Err(error);
        }
        tokens.push(token);
    }
    Ok((context, tokens))
}
