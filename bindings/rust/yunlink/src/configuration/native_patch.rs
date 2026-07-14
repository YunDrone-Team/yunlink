use yunlink_sys as sys;

use super::{ConfigFieldValue, ConfigValue};

pub(crate) fn string_view(value: &str) -> sys::yunlink_string_view_t {
    sys::yunlink_string_view_t {
        data: value.as_ptr().cast(),
        size: value.len(),
    }
}

pub(crate) struct NativePatch<'a> {
    pub updates: Vec<sys::yunlink_config_field_value_view_t>,
    string_lists: Vec<Vec<sys::yunlink_string_view_t>>,
    _marker: std::marker::PhantomData<&'a [ConfigFieldValue]>,
}

impl<'a> NativePatch<'a> {
    pub fn new(source: &'a [ConfigFieldValue]) -> Self {
        let mut output = Self {
            updates: Vec::with_capacity(source.len()),
            string_lists: Vec::new(),
            _marker: std::marker::PhantomData,
        };
        for item in source {
            let value = match &item.value {
                ConfigValue::Bool(value) => sys::yunlink_config_value_view_t {
                    type_: sys::YUNLINK_CONFIG_VALUE_BOOL,
                    bool_value: u8::from(*value),
                    ..Default::default()
                },
                ConfigValue::Int64(value) => sys::yunlink_config_value_view_t {
                    type_: sys::YUNLINK_CONFIG_VALUE_INT64,
                    int64_value: *value,
                    ..Default::default()
                },
                ConfigValue::Double(value) => sys::yunlink_config_value_view_t {
                    type_: sys::YUNLINK_CONFIG_VALUE_DOUBLE,
                    double_value: *value,
                    ..Default::default()
                },
                ConfigValue::String(value) => sys::yunlink_config_value_view_t {
                    type_: sys::YUNLINK_CONFIG_VALUE_STRING,
                    string_value: string_view(value),
                    ..Default::default()
                },
                ConfigValue::StringList(values) => {
                    output
                        .string_lists
                        .push(values.iter().map(|value| string_view(value)).collect());
                    let list = output.string_lists.last().expect("just pushed");
                    sys::yunlink_config_value_view_t {
                        type_: sys::YUNLINK_CONFIG_VALUE_STRING_LIST,
                        string_list: list.as_ptr(),
                        string_list_count: list.len(),
                        ..Default::default()
                    }
                }
            };
            output.updates.push(sys::yunlink_config_field_value_view_t {
                path: string_view(&item.path),
                value,
            });
        }
        output
    }
}
