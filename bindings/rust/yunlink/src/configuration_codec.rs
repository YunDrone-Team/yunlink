//! Deterministic little-endian codecs matching YunLink Core Configuration payloads.

use crate::{
    configuration::*, configuration_codec_io::*, configuration_variants::*, Error, Result,
};

pub trait ConfigurationPayload: Sized {
    fn encode(&self) -> Result<Vec<u8>>;
    fn decode(bytes: &[u8]) -> Result<Self>;
}

fn encode_payload(
    write: impl FnOnce(&mut Writer) -> std::result::Result<(), ()>,
) -> Result<Vec<u8>> {
    let mut writer = Writer::new();
    write(&mut writer).map_err(|_| Error { code: 6 })?;
    writer.finish().map_err(|_| Error { code: 6 })
}
fn decode_payload<T>(
    bytes: &[u8],
    read: impl FnOnce(&mut Reader<'_>) -> std::result::Result<T, ()>,
) -> Result<T> {
    let mut reader = Reader::new(bytes);
    let value = read(&mut reader).map_err(|_| Error { code: 6 })?;
    reader.done().then_some(value).ok_or(Error { code: 6 })
}

impl ConfigurationPayload for ConfigResourceListRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.u8(0);
            Ok(())
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let _ = reader.u8()?;
            Ok(Self)
        })
    }
}

impl ConfigurationPayload for ConfigResourceListResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            writer.list(&self.resources, write_descriptor)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                resources: reader.list(read_descriptor)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceDescribeRequest {
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

impl ConfigurationPayload for ConfigResourceDescribeResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_descriptor(writer, &self.resource)?;
            writer.list(&self.fields, write_schema)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                resource: read_descriptor(reader)?,
                fields: reader.list(read_schema)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceGetRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceGetResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_snapshot(writer, &self.snapshot)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                snapshot: read_snapshot(reader)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourcePatchRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.variant_id)?;
            writer.text(&self.expected_revision)?;
            writer.list(&self.updates, write_field_value)?;
            writer.boolean(self.validate_only);
            Ok(())
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                variant_id: reader.text()?,
                expected_revision: reader.text()?,
                updates: reader.list(read_field_value)?,
                validate_only: reader.boolean()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourcePatchResponse {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            write_status(writer, self.status, &self.message)?;
            write_snapshot(writer, &self.snapshot)?;
            writer.boolean(self.candidate_snapshot.is_some());
            if let Some(candidate_snapshot) = &self.candidate_snapshot {
                write_snapshot(writer, candidate_snapshot)?;
            }
            writer.list(&self.errors, write_error)?;
            write_effects(writer, &self.effects)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            let (status, message) = read_status(reader)?;
            Ok(Self {
                status,
                message,
                snapshot: read_snapshot(reader)?,
                candidate_snapshot: if reader.boolean()? {
                    Some(read_snapshot(reader)?)
                } else {
                    None
                },
                errors: reader.list(read_error)?,
                effects: read_effects(reader)?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceApplyRequest {
    fn encode(&self) -> Result<Vec<u8>> {
        encode_payload(|writer| {
            writer.text(&self.resource_id)?;
            writer.text(&self.expected_revision)
        })
    }
    fn decode(bytes: &[u8]) -> Result<Self> {
        decode_payload(bytes, |reader| {
            Ok(Self {
                resource_id: reader.text()?,
                expected_revision: reader.text()?,
            })
        })
    }
}

impl ConfigurationPayload for ConfigResourceApplyResponse {
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

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn patch_matches_cross_language_golden_vector_and_rejects_corruption() {
        let request = ConfigResourcePatchRequest {
            resource_id: "sunray.params.flight".to_owned(),
            variant_id: "indoor".to_owned(),
            expected_revision: "rev-7".to_owned(),
            updates: vec![
                ConfigFieldValue {
                    path: "control.max_speed".to_owned(),
                    value: ConfigValue::Double(3.5),
                },
                ConfigFieldValue {
                    path: "control.enabled".to_owned(),
                    value: ConfigValue::Bool(true),
                },
            ],
            validate_only: true,
        };
        let expected = vec![
            0x14, 0x00, b's', b'u', b'n', b'r', b'a', b'y', b'.', b'p', b'a', b'r', b'a',
            b'm', b's', b'.', b'f', b'l', b'i', b'g', b'h', b't', 0x06, 0x00, b'i', b'n',
            b'd', b'o', b'o', b'r', 0x05, 0x00, b'r', b'e', b'v', b'-', b'7', 0x02, 0x00,
            0x11, 0x00, b'c', b'o', b'n', b't', b'r', b'o', b'l', b'.', b'm', b'a', b'x',
            b'_', b's', b'p', b'e', b'e', b'd', 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x0c, 0x40, 0x0f, 0x00, b'c', b'o', b'n', b't', b'r', b'o', b'l', b'.', b'e',
            b'n', b'a', b'b', b'l', b'e', b'd', 0x01, 0x01, 0x01,
        ];
        assert_eq!(request.encode().unwrap(), expected);
        assert_eq!(ConfigResourcePatchRequest::decode(&expected).unwrap(), request);
        let mut trailing = expected.clone();
        trailing.push(0);
        assert!(ConfigResourcePatchRequest::decode(&trailing).is_err());
        assert!(ConfigResourcePatchRequest::decode(&expected[..expected.len() - 1]).is_err());
    }

    #[test]
    fn configuration_variants_and_schema_round_trip() {
        let response = ConfigResourceDescribeResponse {
            status: ConfigServiceStatus::Ok,
            message: "ok".to_owned(),
            resource: ConfigResourceDescriptor {
                id: "sunray.params.flight".to_owned(),
                title: "Flight".to_owned(),
                description: String::new(),
                readable: true,
                writable: true,
                apply_supported: true,
                variants_supported: true,
            },
            fields: vec![ConfigFieldSchema {
                path: "control.max_speed".to_owned(),
                group_path: "control".to_owned(),
                title: "Maximum speed".to_owned(),
                description: String::new(),
                value_type: ConfigValueType::Double,
                required: false,
                read_only: false,
                sensitive: false,
                minimum: Some(0.0),
                maximum: Some(10.0),
                validation_pattern: String::new(),
                choices: vec![ConfigChoice {
                    value: ConfigValue::Double(3.0),
                    label: "Indoor".to_owned(),
                }],
                update_policy: ConfigFieldUpdatePolicy::HotReload,
                unit: "m/s".to_owned(),
            }],
        };
        let payload = response.encode().unwrap();
        assert_eq!(ConfigResourceDescribeResponse::decode(&payload).unwrap(), response);
        let variants = ConfigResourceVariantListResponse {
            status: ConfigServiceStatus::Ok,
            message: "ok".to_owned(),
            active_variant_id: "indoor".to_owned(),
            variants: vec![ConfigVariantDescriptor {
                id: "indoor".to_owned(),
                title: "Indoor".to_owned(),
                revision: "r1".to_owned(),
                modified_at_ns: 42,
                active: true,
                mutable_variant: true,
            }],
        };
        assert_eq!(
            ConfigResourceVariantListResponse::decode(&variants.encode().unwrap()).unwrap(),
            variants
        );

        let current = ConfigSnapshot {
            resource_id: "sunray.params.flight".to_owned(),
            revision: "r1".to_owned(),
            applied_revision: "r1".to_owned(),
            variant_id: "indoor".to_owned(),
            active_variant_id: "indoor".to_owned(),
            values: vec![ConfigFieldValue {
                path: "control.max_speed".to_owned(),
                value: ConfigValue::Double(3.0),
            }],
        };
        let preview = ConfigResourcePatchResponse {
            status: ConfigServiceStatus::Ok,
            message: "validated".to_owned(),
            snapshot: current.clone(),
            candidate_snapshot: Some(ConfigSnapshot {
                revision: "candidate-2".to_owned(),
                values: vec![ConfigFieldValue {
                    path: "control.max_speed".to_owned(),
                    value: ConfigValue::Double(3.5),
                }],
                ..current
            }),
            errors: Vec::new(),
            effects: ConfigEffects::default(),
        };
        let preview_expected = vec![
            0x00, 0x09, 0x00, b'v', b'a', b'l', b'i', b'd', b'a', b't', b'e', b'd',
            0x14, 0x00, b's', b'u', b'n', b'r', b'a', b'y', b'.', b'p', b'a', b'r',
            b'a', b'm', b's', b'.', b'f', b'l', b'i', b'g', b'h', b't', 0x02, 0x00,
            b'r', b'1', 0x02, 0x00, b'r', b'1', 0x06, 0x00, b'i', b'n', b'd', b'o',
            b'o', b'r', 0x06, 0x00, b'i', b'n', b'd', b'o', b'o', b'r', 0x01, 0x00,
            0x11, 0x00, b'c', b'o', b'n', b't', b'r', b'o', b'l', b'.', b'm', b'a',
            b'x', b'_', b's', b'p', b'e', b'e', b'd', 0x03, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x08, 0x40, 0x01, 0x14, 0x00, b's', b'u', b'n', b'r', b'a',
            b'y', b'.', b'p', b'a', b'r', b'a', b'm', b's', b'.', b'f', b'l', b'i',
            b'g', b'h', b't', 0x0b, 0x00, b'c', b'a', b'n', b'd', b'i', b'd', b'a',
            b't', b'e', b'-', b'2', 0x02, 0x00, b'r', b'1', 0x06, 0x00, b'i', b'n',
            b'd', b'o', b'o', b'r', 0x06, 0x00, b'i', b'n', b'd', b'o', b'o', b'r',
            0x01, 0x00, 0x11, 0x00, b'c', b'o', b'n', b't', b'r', b'o', b'l', b'.',
            b'm', b'a', b'x', b'_', b's', b'p', b'e', b'e', b'd', 0x03, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ];
        assert_eq!(preview.encode().unwrap(), preview_expected);
        assert_eq!(
            ConfigResourcePatchResponse::decode(&preview_expected).unwrap(),
            preview
        );
    }
}
