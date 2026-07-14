use yunlink_sys as sys;

use super::{
    ConfigApplyRequirement, ConfigEffects, ConfigFieldValue, ConfigResourceDescriptor,
    ConfigSnapshot, ConfigValue,
};

pub(super) unsafe fn string_from_view(view: sys::yunlink_string_view_t) -> Option<String> {
    if view.size == 0 {
        return Some(String::new());
    }
    if view.data.is_null() {
        return None;
    }
    let bytes = unsafe { std::slice::from_raw_parts(view.data.cast::<u8>(), view.size) };
    Some(String::from_utf8_lossy(bytes).into_owned())
}

pub(super) unsafe fn slice_from_raw<'a, T>(pointer: *const T, count: usize) -> Option<&'a [T]> {
    if count == 0 {
        return Some(&[]);
    }
    if pointer.is_null() {
        return None;
    }
    Some(unsafe { std::slice::from_raw_parts(pointer, count) })
}

pub(super) unsafe fn value_from_view(
    view: sys::yunlink_config_value_view_t,
) -> Option<ConfigValue> {
    match view.type_ {
        sys::YUNLINK_CONFIG_VALUE_BOOL => Some(ConfigValue::Bool(view.bool_value != 0)),
        sys::YUNLINK_CONFIG_VALUE_INT64 => Some(ConfigValue::Int64(view.int64_value)),
        sys::YUNLINK_CONFIG_VALUE_DOUBLE => Some(ConfigValue::Double(view.double_value)),
        sys::YUNLINK_CONFIG_VALUE_STRING => Some(ConfigValue::String(unsafe {
            string_from_view(view.string_value)
        }?)),
        sys::YUNLINK_CONFIG_VALUE_STRING_LIST => {
            let values = unsafe { slice_from_raw(view.string_list, view.string_list_count) }?
                .iter()
                .map(|item| unsafe { string_from_view(*item) })
                .collect::<Option<Vec<_>>>()?;
            Some(ConfigValue::StringList(values))
        }
        _ => None,
    }
}

pub(super) unsafe fn descriptor_from_view(
    view: sys::yunlink_config_resource_descriptor_view_t,
) -> Option<ConfigResourceDescriptor> {
    Some(ConfigResourceDescriptor {
        id: unsafe { string_from_view(view.id) }?,
        title: unsafe { string_from_view(view.title) }?,
        description: unsafe { string_from_view(view.description) }?,
        readable: view.readable != 0,
        writable: view.writable != 0,
        apply_supported: view.apply_supported != 0,
    })
}

pub(super) unsafe fn snapshot_from_view(
    view: sys::yunlink_config_snapshot_view_t,
) -> Option<ConfigSnapshot> {
    let values = unsafe { slice_from_raw(view.values, view.value_count) }?
        .iter()
        .map(|item| {
            Some(ConfigFieldValue {
                path: unsafe { string_from_view(item.path) }?,
                value: unsafe { value_from_view(item.value) }?,
            })
        })
        .collect::<Option<Vec<_>>>()?;
    Some(ConfigSnapshot {
        resource_id: unsafe { string_from_view(view.resource_id) }?,
        revision: unsafe { string_from_view(view.revision) }?,
        applied_revision: unsafe { string_from_view(view.applied_revision) }?,
        values,
    })
}

pub(super) unsafe fn effects_from_view(
    view: sys::yunlink_config_effects_view_t,
) -> Option<ConfigEffects> {
    let affected_components =
        unsafe { slice_from_raw(view.affected_components, view.affected_component_count) }?
            .iter()
            .map(|item| unsafe { string_from_view(*item) })
            .collect::<Option<Vec<_>>>()?;
    Some(ConfigEffects {
        requirement: ConfigApplyRequirement::from_native(view.requirement),
        affected_components,
        reconnect_expected: view.reconnect_expected != 0,
    })
}
