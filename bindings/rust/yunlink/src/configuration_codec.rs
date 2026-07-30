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

include!("configuration_codec/resources.rs");
include!("configuration_codec/variants.rs");

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
            0x14, 0x00, b's', b'u', b'n', b'r', b'a', b'y', b'.', b'p', b'a', b'r', b'a', b'm',
            b's', b'.', b'f', b'l', b'i', b'g', b'h', b't', 0x06, 0x00, b'i', b'n', b'd', b'o',
            b'o', b'r', 0x05, 0x00, b'r', b'e', b'v', b'-', b'7', 0x02, 0x00, 0x11, 0x00, b'c',
            b'o', b'n', b't', b'r', b'o', b'l', b'.', b'm', b'a', b'x', b'_', b's', b'p', b'e',
            b'e', b'd', 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x0f, 0x00, b'c',
            b'o', b'n', b't', b'r', b'o', b'l', b'.', b'e', b'n', b'a', b'b', b'l', b'e', b'd',
            0x01, 0x01, 0x01,
        ];
        assert_eq!(request.encode().unwrap(), expected);
        assert_eq!(
            ConfigResourcePatchRequest::decode(&expected).unwrap(),
            request
        );
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
        assert_eq!(
            ConfigResourceDescribeResponse::decode(&payload).unwrap(),
            response
        );
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
            0x00, 0x09, 0x00, b'v', b'a', b'l', b'i', b'd', b'a', b't', b'e', b'd', 0x14, 0x00,
            b's', b'u', b'n', b'r', b'a', b'y', b'.', b'p', b'a', b'r', b'a', b'm', b's', b'.',
            b'f', b'l', b'i', b'g', b'h', b't', 0x02, 0x00, b'r', b'1', 0x02, 0x00, b'r', b'1',
            0x06, 0x00, b'i', b'n', b'd', b'o', b'o', b'r', 0x06, 0x00, b'i', b'n', b'd', b'o',
            b'o', b'r', 0x01, 0x00, 0x11, 0x00, b'c', b'o', b'n', b't', b'r', b'o', b'l', b'.',
            b'm', b'a', b'x', b'_', b's', b'p', b'e', b'e', b'd', 0x03, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x08, 0x40, 0x01, 0x14, 0x00, b's', b'u', b'n', b'r', b'a', b'y', b'.',
            b'p', b'a', b'r', b'a', b'm', b's', b'.', b'f', b'l', b'i', b'g', b'h', b't', 0x0b,
            0x00, b'c', b'a', b'n', b'd', b'i', b'd', b'a', b't', b'e', b'-', b'2', 0x02, 0x00,
            b'r', b'1', 0x06, 0x00, b'i', b'n', b'd', b'o', b'o', b'r', 0x06, 0x00, b'i', b'n',
            b'd', b'o', b'o', b'r', 0x01, 0x00, 0x11, 0x00, b'c', b'o', b'n', b't', b'r', b'o',
            b'l', b'.', b'm', b'a', b'x', b'_', b's', b'p', b'e', b'e', b'd', 0x03, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x0c, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        ];
        assert_eq!(preview.encode().unwrap(), preview_expected);
        assert_eq!(
            ConfigResourcePatchResponse::decode(&preview_expected).unwrap(),
            preview
        );
    }
}
