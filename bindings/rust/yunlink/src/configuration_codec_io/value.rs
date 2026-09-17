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
        // 删除覆写指令：只有类型字节，没有 payload。
        ConfigValue::Unset => {}
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
        // 删除覆写指令：只有类型字节，没有 payload。
        7 => Ok(ConfigValue::Unset),
        _ => Err(()),
    }
}

