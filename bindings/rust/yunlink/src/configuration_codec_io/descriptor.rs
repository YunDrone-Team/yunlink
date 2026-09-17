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
    writer.text(&value.unit)?;
    // 默认值是可选下发：has_default_value 为 false 时没有 payload。
    // 不能靠 default_value 是否为空来判断 —— 默认值可能是空串 / 0 / false。
    writer.boolean(value.has_default_value);
    if value.has_default_value {
        write_value(writer, &value.default_value)?;
    }
    writer.boolean(value.advanced);
    Ok(())
}

pub(crate) fn read_schema(reader: &mut Reader<'_>) -> Result<ConfigFieldSchema, ()> {
    let path = reader.text()?;
    let title = reader.text()?;
    let description = reader.text()?;
    // schema 的 type 不接受 Unset：它是 patch 写入指令，不是字段类型。
    let value_type = schema_value_type(reader.u8()?)?;
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
    let validation_pattern = reader.text()?;
    let choices = reader.list(|reader| {
        Ok(ConfigChoice {
            value: read_value(reader)?,
            label: reader.text()?,
        })
    })?;
    let group_path = reader.text()?;
    let update_policy = update_policy(reader.u8()?)?;
    let unit = reader.text()?;
    let has_default_value = reader.boolean()?;
    let default_value = if has_default_value {
        read_value(reader)?
    } else {
        // 与 C++ 端 ConfigValue 的默认构造保持一致（type = String、值为空）；
        // 是否有效完全由 has_default_value 决定。
        ConfigValue::String(String::new())
    };
    let advanced = reader.boolean()?;
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
        validation_pattern,
        choices,
        group_path,
        update_policy,
        unit,
        default_value,
        has_default_value,
        advanced,
    })
}

