struct CallbackContext {
    sender: broadcast::Sender<Event>,
}

unsafe fn copy_string(value: sys::yunlink_v2_string_view_t) -> String {
    if value.data.is_null() || value.len == 0 {
        return String::new();
    }
    String::from_utf8_lossy(slice::from_raw_parts(value.data.cast::<u8>(), value.len)).into_owned()
}

unsafe fn copy_bytes(value: sys::yunlink_v2_bytes_view_t) -> Vec<u8> {
    if value.data.is_null() || value.len == 0 {
        return Vec::new();
    }
    slice::from_raw_parts(value.data, value.len).to_vec()
}

unsafe fn copy_message(event: &sys::yunlink_v2_event_t) -> Message {
    let target_uids = if event.target.uids.is_null() || event.target.uid_count == 0 {
        Vec::new()
    } else {
        slice::from_raw_parts(event.target.uids, event.target.uid_count)
            .iter()
            .map(|value| copy_string(*value))
            .collect()
    };
    let target = match event.target.scope {
        1 => Target::Endpoint(target_uids),
        2 => Target::Entity(target_uids),
        3 => Target::Group(target_uids),
        _ => Target::Broadcast,
    };
    Message {
        peer_id: copy_string(event.peer_id),
        session_id: event.session_id,
        operation: event.operation,
        qos: event.qos_class,
        message_id: event.message_id,
        correlation_id: event.correlation_id,
        created_at_ms: event.created_at_ms,
        ttl_ms: event.ttl_ms,
        source_endpoint_uid: copy_string(event.source_endpoint_uid),
        source_entity_uid: copy_string(event.source_entity_uid),
        target,
        type_ref: TypeRef {
            profile_id: copy_string(event.type_ref.profile_id),
            major: event.type_ref.major,
            minor: event.type_ref.minor,
            type_name: copy_string(event.type_ref.type_name),
        },
        payload: copy_bytes(event.payload),
    }
}

unsafe extern "C" fn event_callback(event: *const sys::yunlink_v2_event_t, user_data: *mut c_void) {
    if event.is_null() || user_data.is_null() {
        return;
    }
    let event = &*event;
    let context = &*(user_data.cast::<CallbackContext>());
    let parsed = match event.kind {
        1 => {
            let message = copy_message(event);
            match event.family {
                2 => Event::Authority(message),
                3 => Event::EntityDirectory(message),
                4 => Event::Stream(message),
                5 => Event::Action(message),
                6 => Event::Rpc(message),
                7 => Event::Configuration(message),
                8 => Event::Log(message),
                9 => Event::Bulk(message),
                _ => return,
            }
        }
        2 => Event::Link {
            peer_id: copy_string(event.peer_id),
            up: event.link_up != 0,
        },
        3 => Event::Session {
            peer_id: copy_string(event.peer_id),
            session_id: event.session_id,
            state: event.session_state,
            authenticated: event.session_authenticated != 0,
        },
        4 => Event::Error {
            peer_id: copy_string(event.peer_id),
            code: event.error_code,
            message: copy_string(event.message),
        },
        _ => return,
    };
    let _ = context.sender.send(parsed);
}

fn string_view(value: &str) -> sys::yunlink_v2_string_view_t {
    sys::yunlink_v2_string_view_t {
        data: value.as_ptr().cast(),
        len: value.len(),
    }
}

fn profile_view(value: &Profile) -> sys::yunlink_v2_profile_view_t {
    sys::yunlink_v2_profile_view_t {
        profile_id: string_view(&value.profile_id),
        major: value.major,
        minor: value.minor,
        schema_digest: string_view(&value.schema_digest),
    }
}

fn c_buffer(value: &[std::ffi::c_char]) -> String {
    let len = value
        .iter()
        .position(|byte| *byte == 0)
        .unwrap_or(value.len());
    String::from_utf8_lossy(unsafe { slice::from_raw_parts(value.as_ptr().cast::<u8>(), len) })
        .into_owned()
}
