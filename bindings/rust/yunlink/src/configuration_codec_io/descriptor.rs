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
