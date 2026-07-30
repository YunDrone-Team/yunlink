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
