pub mod org {
    pub mod yunlink {
        pub mod mobility {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.mobility.v1.rs"));
            }
        }
        pub mod telemetry {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.telemetry.v1.rs"));
            }
        }
    }
}

pub mod com {
    pub mod yundrone {
        pub mod sunray {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/com.yundrone.sunray.v1.rs"));
            }
        }
    }
}

pub use com::yundrone::sunray::v1 as sunray;
pub use org::yunlink::mobility::v1 as mobility;
pub use org::yunlink::telemetry::v1 as telemetry;

pub const MOBILITY_PROFILE_ID: &str = "org.yunlink.mobility";
pub const TELEMETRY_PROFILE_ID: &str = "org.yunlink.telemetry";
pub const SUNRAY_PROFILE_ID: &str = "com.yundrone.sunray";

pub const SUMMARY_MAX_METRICS: usize = 64;
pub const SUMMARY_MAX_PAYLOAD_BYTES: usize = 16 * 1024;

pub fn valid_metric_key(key: &str) -> bool {
    if key.is_empty() || key.len() > 128 {
        return false;
    }
    let segments = key.split('.').collect::<Vec<_>>();
    segments.len() >= 3
        && segments.iter().all(|segment| {
            let mut chars = segment.chars();
            chars.next().is_some_and(|value| value.is_ascii_lowercase())
                && chars.all(|value| {
                    value.is_ascii_lowercase() || value.is_ascii_digit() || value == '_'
                })
        })
}

pub fn validate_summary_snapshot(
    snapshot: &telemetry::SummarySnapshot,
) -> Result<(), &'static str> {
    use std::collections::HashSet;
    use telemetry::metric_value::Value;

    if snapshot.metrics.len() > SUMMARY_MAX_METRICS {
        return Err("too many metrics");
    }
    if prost::Message::encoded_len(snapshot) > SUMMARY_MAX_PAYLOAD_BYTES {
        return Err("summary payload exceeds limit");
    }
    let mut keys = HashSet::new();
    for metric in &snapshot.metrics {
        if !valid_metric_key(&metric.key) {
            return Err("invalid metric key");
        }
        if !keys.insert(metric.key.as_str()) {
            return Err("duplicate metric key");
        }
        if metric.unit.len() > 16 {
            return Err("metric unit exceeds limit");
        }
        let quality = telemetry::MetricQuality::try_from(metric.quality)
            .map_err(|_| "invalid metric quality")?;
        if quality == telemetry::MetricQuality::Unspecified {
            return Err("invalid metric quality");
        }
        if matches!(
            quality,
            telemetry::MetricQuality::MetricValid | telemetry::MetricQuality::MetricStale
        ) && metric
            .value
            .as_ref()
            .and_then(|value| value.value.as_ref())
            .is_none()
        {
            return Err("metric value is required");
        }
        match metric.value.as_ref().and_then(|value| value.value.as_ref()) {
            Some(Value::DoubleValue(value)) if !value.is_finite() => {
                return Err("metric double is not finite")
            }
            Some(Value::EnumToken(value)) if value.len() > 64 => {
                return Err("metric enum token exceeds limit")
            }
            Some(Value::TextValue(value)) if value.len() > 256 => {
                return Err("metric text exceeds limit")
            }
            _ => {}
        }
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use prost::Message;

    use super::{mobility, sunray, telemetry, validate_summary_snapshot};

    const GOTO_GOLDEN: &[u8] = &[
        0x0a, 0x03, 0x6d, 0x61, 0x70, 0x12, 0x1b, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0,
        0x3f, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x19, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0xe0, 0x3f, 0x19, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xd0, 0x3f,
    ];

    #[test]
    fn profile_payloads_match_cross_language_golden_vectors() {
        let goal = mobility::GotoGoal {
            frame_id: "map".into(),
            position: Some(mobility::Vector3 {
                x: 1.0,
                y: -2.0,
                z: 0.5,
            }),
            yaw_rad: 0.25,
        };
        assert_eq!(goal.encode_to_vec(), GOTO_GOLDEN);

        let request = sunray::FeatureStartRequest {
            name: "mapping".into(),
        };
        assert_eq!(request.encode_to_vec(), b"\x0a\x07mapping");
        assert!(sunray::FeatureStartRequest::decode(b"\x0a\x08mapping".as_slice()).is_err());

        let summary = telemetry::SummarySnapshot {
            generated_at_ns: 1,
            metrics: vec![telemetry::Metric {
                key: "org.test.ready".into(),
                value: Some(telemetry::MetricValue {
                    value: Some(telemetry::metric_value::Value::BoolValue(true)),
                }),
                unit: String::new(),
                quality: telemetry::MetricQuality::MetricValid as i32,
                source_timestamp_ns: 2,
            }],
        };
        assert!(validate_summary_snapshot(&summary).is_ok());
        assert_eq!(
            summary.encode_to_vec(),
            b"\x08\x01\x12\x18\x0a\x0eorg.test.ready\x12\x02\x08\x01\x20\x01\x28\x02"
        );
    }

    #[test]
    fn summary_validation_rejects_invalid_payloads() {
        let metric = |key: &str, value| telemetry::Metric {
            key: key.into(),
            value: Some(telemetry::MetricValue { value: Some(value) }),
            unit: String::new(),
            quality: telemetry::MetricQuality::MetricValid as i32,
            source_timestamp_ns: 0,
        };
        let mut summary = telemetry::SummarySnapshot {
            generated_at_ns: 0,
            metrics: vec![metric(
                "org.test.value",
                telemetry::metric_value::Value::IntValue(7),
            )],
        };
        assert!(validate_summary_snapshot(&summary).is_ok());
        summary.metrics.push(summary.metrics[0].clone());
        assert_eq!(
            validate_summary_snapshot(&summary),
            Err("duplicate metric key")
        );
        summary.metrics.truncate(1);
        summary.metrics[0].key = "Org.test.value".into();
        assert_eq!(
            validate_summary_snapshot(&summary),
            Err("invalid metric key")
        );
        summary.metrics[0] = metric(
            "org.test.value",
            telemetry::metric_value::Value::DoubleValue(f64::NAN),
        );
        assert_eq!(
            validate_summary_snapshot(&summary),
            Err("metric double is not finite")
        );
    }
}
