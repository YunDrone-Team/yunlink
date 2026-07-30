impl ConfigurationPayload for ConfigResourceVariantListRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| writer.text(&self.resource_id))
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
            })
        })
    }
}
impl ConfigurationPayload for ConfigResourceVariantListResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            writer.text(&self.active_variant_id)?;
            writer.list(&self.variants, write_variant)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                active_variant_id: reader.text()?,
                variants: reader.list(read_variant)?,
            })
        })
    }
}

fn write_variant_create(
    writer: &mut Writer,
    resource_id: &str,
    variant_id: &str,
    source: ConfigVariantSource,
    revision: &str,
) -> std::result::Result<(), ()> {
    writer.text(resource_id)?;
    writer.text(variant_id)?;
    writer.u8(source as u8);
    writer.text(revision)
}
fn read_variant_create(
    reader: &mut Reader<'_>,
) -> std::result::Result<(String, String, ConfigVariantSource, String), ()> {
    Ok((
        reader.text()?,
        reader.text()?,
        variant_source(reader.u8()?)?,
        reader.text()?,
    ))
}
impl ConfigurationPayload for ConfigResourceVariantCreateRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_variant_create(
                writer,
                &self.resource_id,
                &self.variant_id,
                self.source,
                &self.expected_active_revision,
            )
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (resource_id, variant_id, source, expected_active_revision) =
                read_variant_create(reader)?;
            Ok(Self {
                resource_id,
                variant_id,
                source,
                expected_active_revision,
            })
        })
    }
}

macro_rules! impl_variant_mutation_response {
    ($type:ty) => {
        impl ConfigurationPayload for $type {
            fn encode(&self) -> Result<Vec<u8>> {
                encode_payload(|writer| {
                    write_status(writer, self.status, &self.message)?;
                    write_variant(writer, &self.variant)?;
                    write_effects(writer, &self.effects)
                })
            }
            fn decode(bytes: &[u8]) -> Result<Self> {
                decode_payload(bytes, |reader| {
                    let (status, message) = read_status(reader)?;
                    Ok(Self {
                        status,
                        message,
                        variant: read_variant(reader)?,
                        effects: read_effects(reader)?,
                    })
                })
            }
        }
    };
}
impl_variant_mutation_response!(ConfigResourceVariantCreateResponse);
impl_variant_mutation_response!(ConfigResourceVariantSaveCurrentResponse);

impl ConfigurationPayload for ConfigResourceVariantSaveCurrentRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)?;
            writer.text(&self.expected_variant_revision)?;
            writer.text(&self.expected_active_revision)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
                expected_variant_revision: reader.text()?,
                expected_active_revision: reader.text()?,
            })
        })
    }
}
impl ConfigurationPayload for ConfigResourceVariantActivateRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)?;
            writer.text(&self.expected_active_revision)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
                expected_active_revision: reader.text()?,
            })
        })
    }
}
impl ConfigurationPayload for ConfigResourceVariantActivateResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            writer.text(&self.applied_revision)?;
            writer.u8(self.outcome as u8);
            write_effects(writer, &self.effects)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                applied_revision: reader.text()?,
                outcome: outcome(reader.u8()?)?,
                effects: read_effects(reader)?,
            })
        })
    }
}
impl ConfigurationPayload for ConfigResourceVariantDeleteRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)?;
            writer.text(&self.expected_revision)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
                expected_revision: reader.text()?,
            })
        })
    }
}
impl ConfigurationPayload for ConfigResourceVariantDeleteResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| write_status(writer, self.status, &self.message))
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self { status, message })
        })
    }
}
