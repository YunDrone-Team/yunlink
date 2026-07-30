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
