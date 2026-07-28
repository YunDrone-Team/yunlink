use crate::configuration::*;

pub(crate) const MAX_CONFIG_ITEMS: usize = 256;
const MAX_STRING_BYTES: usize = 1024;

pub(crate) struct Writer {
    data: Vec<u8>,
    valid: bool,
}

impl Writer {
    pub(crate) fn new() -> Self {
        Self {
            data: Vec::new(),
            valid: true,
        }
    }
    pub(crate) fn finish(self) -> Result<Vec<u8>, ()> {
        self.valid.then_some(self.data).ok_or(())
    }
    pub(crate) fn u8(&mut self, value: u8) {
        self.data.push(value);
    }
    pub(crate) fn boolean(&mut self, value: bool) {
        self.u8(u8::from(value));
    }
    pub(crate) fn u16(&mut self, value: u16) {
        self.data.extend(value.to_le_bytes());
    }
    pub(crate) fn u64(&mut self, value: u64) {
        self.data.extend(value.to_le_bytes());
    }
    pub(crate) fn f64(&mut self, value: f64) {
        if !value.is_finite() {
            self.valid = false;
            return;
        }
        self.u64(value.to_bits());
    }
    pub(crate) fn text(&mut self, value: &str) -> Result<(), ()> {
        if value.len() > MAX_STRING_BYTES {
            return Err(());
        }
        self.u16(value.len() as u16);
        self.data.extend(value.as_bytes());
        Ok(())
    }
    pub(crate) fn list<T>(
        &mut self,
        values: &[T],
        write: impl Fn(&mut Self, &T) -> Result<(), ()>,
    ) -> Result<(), ()> {
        if values.len() > MAX_CONFIG_ITEMS {
            return Err(());
        }
        self.u16(values.len() as u16);
        values.iter().try_for_each(|value| write(self, value))
    }
}

pub(crate) struct Reader<'a> {
    data: &'a [u8],
    cursor: usize,
}

impl<'a> Reader<'a> {
    pub(crate) fn new(data: &'a [u8]) -> Self {
        Self { data, cursor: 0 }
    }
    pub(crate) fn done(&self) -> bool {
        self.cursor == self.data.len()
    }
    pub(crate) fn u8(&mut self) -> Result<u8, ()> {
        let value = *self.data.get(self.cursor).ok_or(())?;
        self.cursor += 1;
        Ok(value)
    }
    pub(crate) fn boolean(&mut self) -> Result<bool, ()> {
        match self.u8()? {
            0 => Ok(false),
            1 => Ok(true),
            _ => Err(()),
        }
    }
    pub(crate) fn u16(&mut self) -> Result<u16, ()> {
        let value = self.data.get(self.cursor..self.cursor + 2).ok_or(())?;
        self.cursor += 2;
        Ok(u16::from_le_bytes([value[0], value[1]]))
    }
    pub(crate) fn u64(&mut self) -> Result<u64, ()> {
        let value = self.data.get(self.cursor..self.cursor + 8).ok_or(())?;
        self.cursor += 8;
        Ok(u64::from_le_bytes(value.try_into().map_err(|_| ())?))
    }
    pub(crate) fn f64(&mut self) -> Result<f64, ()> {
        let value = f64::from_bits(self.u64()?);
        value.is_finite().then_some(value).ok_or(())
    }
    pub(crate) fn text(&mut self) -> Result<String, ()> {
        let length = self.u16()? as usize;
        let value = self.data.get(self.cursor..self.cursor + length).ok_or(())?;
        self.cursor += length;
        String::from_utf8(value.to_vec()).map_err(|_| ())
    }
    pub(crate) fn list<T>(
        &mut self,
        read: impl Fn(&mut Self) -> Result<T, ()>,
    ) -> Result<Vec<T>, ()> {
        let count = self.u16()? as usize;
        if count > MAX_CONFIG_ITEMS {
            return Err(());
        }
        (0..count).map(|_| read(self)).collect()
    }
}

pub(crate) fn write_value(writer: &mut Writer, value: &ConfigValue) -> Result<(), ()> {
    writer.u8(value.value_type() as u8);
    match value {
        ConfigValue::Bool(value) => writer.boolean(*value),
        ConfigValue::Int64(value) => writer.u64(*value as u64),
        ConfigValue::Double(value) => writer.f64(*value),
        ConfigValue::String(value) => writer.text(value)?,
        ConfigValue::StringList(values) => {
            writer.list(values, |writer, value| writer.text(value))?
        }
        ConfigValue::DoubleList(values) => writer.list(values, |writer, value| {
            writer.f64(*value);
            Ok(())
        })?,
    }
    Ok(())
}

pub(crate) fn read_value(reader: &mut Reader<'_>) -> Result<ConfigValue, ()> {
    match reader.u8()? {
        1 => Ok(ConfigValue::Bool(reader.boolean()?)),
        2 => Ok(ConfigValue::Int64(reader.u64()? as i64)),
        3 => Ok(ConfigValue::Double(reader.f64()?)),
        4 => Ok(ConfigValue::String(reader.text()?)),
        5 => Ok(ConfigValue::StringList(
            reader.list(|reader| reader.text())?,
        )),
        6 => Ok(ConfigValue::DoubleList(reader.list(|reader| reader.f64())?)),
        _ => Err(()),
    }
}

pub(crate) fn write_descriptor(
    writer: &mut Writer,
    value: &ConfigResourceDescriptor,
) -> Result<(), ()> {
    writer.text(&value.id)?;
    writer.text(&value.title)?;
    writer.text(&value.description)?;
    writer.boolean(value.readable);
    writer.boolean(value.writable);
    writer.boolean(value.apply_supported);
    writer.boolean(value.variants_supported);
    Ok(())
}

pub(crate) fn read_descriptor(reader: &mut Reader<'_>) -> Result<ConfigResourceDescriptor, ()> {
    Ok(ConfigResourceDescriptor {
        id: reader.text()?,
        title: reader.text()?,
        description: reader.text()?,
        readable: reader.boolean()?,
        writable: reader.boolean()?,
        apply_supported: reader.boolean()?,
        variants_supported: reader.boolean()?,
    })
}

pub(crate) fn write_schema(writer: &mut Writer, value: &ConfigFieldSchema) -> Result<(), ()> {
    writer.text(&value.path)?;
    writer.text(&value.title)?;
    writer.text(&value.description)?;
    writer.u8(value.value_type as u8);
    writer.boolean(value.required);
    writer.boolean(value.read_only);
    writer.boolean(value.sensitive);
    writer.boolean(value.minimum.is_some());
    writer.f64(value.minimum.unwrap_or_default());
    writer.boolean(value.maximum.is_some());
    writer.f64(value.maximum.unwrap_or_default());
    writer.text(&value.validation_pattern)?;
    writer.list(&value.choices, |writer, choice| {
        write_value(writer, &choice.value)?;
        writer.text(&choice.label)
    })?;
    writer.text(&value.group_path)?;
    writer.u8(value.update_policy as u8);
    writer.text(&value.unit)
}

pub(crate) fn read_schema(reader: &mut Reader<'_>) -> Result<ConfigFieldSchema, ()> {
    let path = reader.text()?;
    let title = reader.text()?;
    let description = reader.text()?;
    let value_type = value_type(reader.u8()?)?;
    let required = reader.boolean()?;
    let read_only = reader.boolean()?;
    let sensitive = reader.boolean()?;
    let minimum = reader.boolean()?.then(|| reader.f64()).transpose()?;
    if minimum.is_none() {
        let _ = reader.f64()?;
    }
    let maximum = reader.boolean()?.then(|| reader.f64()).transpose()?;
    if maximum.is_none() {
        let _ = reader.f64()?;
    }
    Ok(ConfigFieldSchema {
        path,
        title,
        description,
        value_type,
        required,
        read_only,
        sensitive,
        minimum,
        maximum,
        validation_pattern: reader.text()?,
        choices: reader.list(|reader| {
            Ok(ConfigChoice {
                value: read_value(reader)?,
                label: reader.text()?,
            })
        })?,
        group_path: reader.text()?,
        update_policy: update_policy(reader.u8()?)?,
        unit: reader.text()?,
    })
}

pub(crate) fn write_field_value(writer: &mut Writer, value: &ConfigFieldValue) -> Result<(), ()> {
    writer.text(&value.path)?;
    write_value(writer, &value.value)
}
pub(crate) fn read_field_value(reader: &mut Reader<'_>) -> Result<ConfigFieldValue, ()> {
    Ok(ConfigFieldValue {
        path: reader.text()?,
        value: read_value(reader)?,
    })
}

pub(crate) fn write_snapshot(writer: &mut Writer, value: &ConfigSnapshot) -> Result<(), ()> {
    writer.text(&value.resource_id)?;
    writer.text(&value.revision)?;
    writer.text(&value.applied_revision)?;
    writer.text(&value.variant_id)?;
    writer.text(&value.active_variant_id)?;
    writer.list(&value.values, write_field_value)
}
pub(crate) fn read_snapshot(reader: &mut Reader<'_>) -> Result<ConfigSnapshot, ()> {
    Ok(ConfigSnapshot {
        resource_id: reader.text()?,
        revision: reader.text()?,
        applied_revision: reader.text()?,
        variant_id: reader.text()?,
        active_variant_id: reader.text()?,
        values: reader.list(read_field_value)?,
    })
}

pub(crate) fn write_error(writer: &mut Writer, value: &ConfigFieldError) -> Result<(), ()> {
    writer.text(&value.path)?;
    writer.text(&value.code)?;
    writer.text(&value.message)
}
pub(crate) fn read_error(reader: &mut Reader<'_>) -> Result<ConfigFieldError, ()> {
    Ok(ConfigFieldError {
        path: reader.text()?,
        code: reader.text()?,
        message: reader.text()?,
    })
}

pub(crate) fn write_effects(writer: &mut Writer, value: &ConfigEffects) -> Result<(), ()> {
    writer.u8(value.requirement as u8);
    writer.list(&value.affected_components, |writer, value| {
        writer.text(value)
    })?;
    writer.boolean(value.reconnect_expected);
    Ok(())
}
pub(crate) fn read_effects(reader: &mut Reader<'_>) -> Result<ConfigEffects, ()> {
    Ok(ConfigEffects {
        requirement: requirement(reader.u8()?)?,
        affected_components: reader.list(|reader| reader.text())?,
        reconnect_expected: reader.boolean()?,
    })
}

pub(crate) fn write_variant(
    writer: &mut Writer,
    value: &ConfigVariantDescriptor,
) -> Result<(), ()> {
    writer.text(&value.id)?;
    writer.text(&value.title)?;
    writer.text(&value.revision)?;
    writer.u64(value.modified_at_ns);
    writer.boolean(value.active);
    writer.boolean(value.mutable_variant);
    Ok(())
}
pub(crate) fn read_variant(reader: &mut Reader<'_>) -> Result<ConfigVariantDescriptor, ()> {
    Ok(ConfigVariantDescriptor {
        id: reader.text()?,
        title: reader.text()?,
        revision: reader.text()?,
        modified_at_ns: reader.u64()?,
        active: reader.boolean()?,
        mutable_variant: reader.boolean()?,
    })
}

pub(crate) fn status(value: u8) -> Result<ConfigServiceStatus, ()> {
    match value {
        0 => Ok(ConfigServiceStatus::Ok),
        1 => Ok(ConfigServiceStatus::NotFound),
        2 => Ok(ConfigServiceStatus::Unsupported),
        3 => Ok(ConfigServiceStatus::Unauthenticated),
        4 => Ok(ConfigServiceStatus::Unauthorized),
        5 => Ok(ConfigServiceStatus::Conflict),
        6 => Ok(ConfigServiceStatus::Invalid),
        7 => Ok(ConfigServiceStatus::UnsafeState),
        8 => Ok(ConfigServiceStatus::InternalError),
        _ => Err(()),
    }
}
pub(crate) fn requirement(value: u8) -> Result<ConfigApplyRequirement, ()> {
    match value {
        0 => Ok(ConfigApplyRequirement::None),
        1 => Ok(ConfigApplyRequirement::ComponentRestart),
        2 => Ok(ConfigApplyRequirement::EndpointRestart),
        3 => Ok(ConfigApplyRequirement::DeviceReboot),
        4 => Ok(ConfigApplyRequirement::Manual),
        _ => Err(()),
    }
}
pub(crate) fn outcome(value: u8) -> Result<ConfigApplyOutcome, ()> {
    match value {
        1 => Ok(ConfigApplyOutcome::Applied),
        2 => Ok(ConfigApplyOutcome::RestartScheduled),
        3 => Ok(ConfigApplyOutcome::ManualActionRequired),
        4 => Ok(ConfigApplyOutcome::Failed),
        _ => Err(()),
    }
}
pub(crate) fn value_type(value: u8) -> Result<ConfigValueType, ()> {
    match value {
        1 => Ok(ConfigValueType::Bool),
        2 => Ok(ConfigValueType::Int64),
        3 => Ok(ConfigValueType::Double),
        4 => Ok(ConfigValueType::String),
        5 => Ok(ConfigValueType::StringList),
        6 => Ok(ConfigValueType::DoubleList),
        _ => Err(()),
    }
}
pub(crate) fn update_policy(value: u8) -> Result<ConfigFieldUpdatePolicy, ()> {
    match value {
        0 => Ok(ConfigFieldUpdatePolicy::HotReload),
        1 => Ok(ConfigFieldUpdatePolicy::ComponentRestart),
        2 => Ok(ConfigFieldUpdatePolicy::EndpointRestart),
        3 => Ok(ConfigFieldUpdatePolicy::DeviceReboot),
        4 => Ok(ConfigFieldUpdatePolicy::Manual),
        _ => Err(()),
    }
}
pub(crate) fn variant_source(value: u8) -> Result<ConfigVariantSource, ()> {
    match value {
        1 => Ok(ConfigVariantSource::Default),
        2 => Ok(ConfigVariantSource::Active),
        _ => Err(()),
    }
}
pub(crate) fn write_status(
    writer: &mut Writer,
    status: ConfigServiceStatus,
    message: &str,
) -> Result<(), ()> {
    writer.u8(status as u8);
    writer.text(message)
}
pub(crate) fn read_status(reader: &mut Reader<'_>) -> Result<(ConfigServiceStatus, String), ()> {
    Ok((status(reader.u8()?)?, reader.text()?))
}
