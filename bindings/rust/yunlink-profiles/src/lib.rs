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
        pub mod system {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.system.v1.rs"));
            }
        }
        pub mod media {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.media.v1.rs"));
            }
        }
        pub mod shell {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.shell.v1.rs"));
            }
        }
    }
}

pub mod com {
    pub mod yundrone {
        pub mod sunray {
            pub mod v2 {
                include!(concat!(env!("OUT_DIR"), "/com.yundrone.sunray.v2.rs"));
            }
        }
    }
}

pub use com::yundrone::sunray::v2 as sunray;
pub use org::yunlink::media::v1 as media;
pub use org::yunlink::mobility::v1 as mobility;
pub use org::yunlink::system::v1 as system;
pub use org::yunlink::telemetry::v1 as telemetry;
pub use org::yunlink::shell::v1 as shell;

pub const MOBILITY_PROFILE_ID: &str = "org.yunlink.mobility";
pub const TELEMETRY_PROFILE_ID: &str = "org.yunlink.telemetry";
pub const MEDIA_PROFILE_ID: &str = "org.yunlink.media";
pub const SUNRAY_PROFILE_ID: &str = "com.yundrone.sunray";
pub const SYSTEM_PROFILE_ID: &str = "org.yunlink.system";
pub const SHELL_PROFILE_ID: &str = "org.yunlink.shell";

pub const SUMMARY_MAX_METRICS: usize = 64;
pub const SUMMARY_MAX_PAYLOAD_BYTES: usize = 16 * 1024;
pub const MIN_DIRECT_CONTROL_LEASE_MS: u32 = 250;
pub const MAX_DIRECT_CONTROL_LEASE_MS: u32 = 2000;
pub const MAX_WAYPOINT_COUNT: usize = 256;
pub const MAX_WAYPOINT_TASK_NAME_BYTES: usize = 96;
pub const MEDIA_MAX_CAMERAS: usize = 32;
pub const MEDIA_MAX_CHUNK_BYTES: usize = 256 * 1024;
pub const MEDIA_MAX_SOURCE_URI_BYTES: usize = 2048;
pub const MEDIA_MAX_ASSET_PAGE_SIZE: usize = 100;
pub const MEDIA_MAX_PAGE_TOKEN_BYTES: usize = 512;
pub const MEDIA_MAX_DIMENSION_PIXELS: u32 = 32768;
pub const MINIMUM_TRUSTED_UNIX_TIME_MS: u64 = 1_704_067_200_000;
pub const MAXIMUM_TRUSTED_UNIX_TIME_MS: u64 = 4_102_444_800_000;

mod validation_media;
mod validation_shell;
mod validation_sunray;
mod validation_system;
mod validation_telemetry;

pub use validation_media::*;
pub use validation_shell::*;
pub use validation_sunray::*;
pub use validation_system::*;
pub use validation_telemetry::*;

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
