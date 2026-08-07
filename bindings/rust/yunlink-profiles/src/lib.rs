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
        pub mod media {
            pub mod v1 {
                include!(concat!(env!("OUT_DIR"), "/org.yunlink.media.v1.rs"));
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
pub use org::yunlink::telemetry::v1 as telemetry;

pub const MOBILITY_PROFILE_ID: &str = "org.yunlink.mobility";
pub const TELEMETRY_PROFILE_ID: &str = "org.yunlink.telemetry";
pub const MEDIA_PROFILE_ID: &str = "org.yunlink.media";
pub const SUNRAY_PROFILE_ID: &str = "com.yundrone.sunray";

pub const SUMMARY_MAX_METRICS: usize = 64;
pub const SUMMARY_MAX_PAYLOAD_BYTES: usize = 16 * 1024;
pub const MIN_DIRECT_CONTROL_LEASE_MS: u32 = 250;
pub const MAX_DIRECT_CONTROL_LEASE_MS: u32 = 2000;
pub const MAX_WAYPOINT_COUNT: usize = 256;
pub const MAX_WAYPOINT_TASK_NAME_BYTES: usize = 96;
pub const MEDIA_MAX_CAMERAS: usize = 32;
pub const MEDIA_MAX_CHUNK_BYTES: usize = 256 * 1024;
pub const MEDIA_MAX_SOURCE_URI_BYTES: usize = 2048;

fn valid_media_token(value: &str, max_bytes: usize) -> bool {
    !value.is_empty()
        && value.len() <= max_bytes
        && value
            .bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'-' | b'_' | b'.'))
}

pub fn validate_camera_descriptor(camera: &media::CameraDescriptor) -> Result<(), &'static str> {
    (valid_media_token(&camera.camera_uid, 96)
        && camera.name.len() <= 128
        && camera.image_topic.len() <= 256
        && camera.camera_info_topic.len() <= 256
        && camera.encoding.len() <= 64
        && camera.error_message.len() <= 256
        && camera.frame_rate_hz.is_finite()
        && camera.frame_rate_hz >= 0.0)
        .then_some(())
        .ok_or("camera descriptor is invalid")
}

pub fn validate_camera_catalog_snapshot(
    snapshot: &media::CameraCatalogSnapshot,
) -> Result<(), &'static str> {
    use std::collections::HashSet;
    if snapshot.cameras.len() > MEDIA_MAX_CAMERAS {
        return Err("camera catalog is invalid");
    }
    let mut camera_uids = HashSet::new();
    for camera in &snapshot.cameras {
        validate_camera_descriptor(camera)?;
        if !camera_uids.insert(camera.camera_uid.as_str()) {
            return Err("duplicate camera uid");
        }
    }
    Ok(())
}

pub fn validate_media_endpoint_descriptor(
    endpoint: &media::MediaEndpointDescriptor,
) -> Result<(), &'static str> {
    let uri = endpoint.uri.as_str();
    let rest = uri
        .strip_prefix("rtsp://")
        .ok_or("media endpoint descriptor is invalid")?;
    let (authority, path) = rest
        .split_once('/')
        .ok_or("media endpoint descriptor is invalid")?;
    let (host, port) = authority
        .rsplit_once(':')
        .ok_or("media endpoint descriptor is invalid")?;
    let port = port
        .parse::<u16>()
        .map_err(|_| "media endpoint descriptor is invalid")?;
    let valid_host = if host.starts_with('[') && host.ends_with(']') {
        host.len() > 2
            && host[1..host.len() - 1]
                .bytes()
                .all(|byte| byte.is_ascii_hexdigit() || matches!(byte, b':' | b'.'))
    } else {
        host.bytes()
            .all(|byte| byte.is_ascii_alphanumeric() || matches!(byte, b'.' | b'-'))
    };
    (endpoint.protocol == "rtsp"
        && !host.is_empty()
        && valid_host
        && port > 0
        && !path.is_empty()
        && uri.len() <= MEDIA_MAX_SOURCE_URI_BYTES
        && !uri.contains('@')
        && !uri.contains('#')
        && !uri.bytes().any(|byte| byte < 0x20 || byte == 0x7f)
        && endpoint.username.len() <= 256
        && endpoint.password.len() <= 256
        && !endpoint
            .username
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f)
        && !endpoint
            .password
            .bytes()
            .any(|byte| byte < 0x20 || byte == 0x7f))
    .then_some(())
    .ok_or("media endpoint descriptor is invalid")
}

pub fn validate_media_asset_ref(asset: &media::MediaAssetRef) -> Result<(), &'static str> {
    (valid_media_token(&asset.asset_id, 128)
        && (media::MediaAssetKind::MediaPhoto as i32..=media::MediaAssetKind::MediaVideo as i32)
            .contains(&asset.kind)
        && !asset.mime_type.is_empty()
        && asset.mime_type.len() <= 96
        && asset.size_bytes > 0
        && asset.sha256.len() == 32
        && valid_media_token(&asset.camera_uid, 96)
        && asset.display_name.len() <= 160)
        .then_some(())
        .ok_or("media asset reference is invalid")
}

pub fn validate_media_asset_chunk(
    chunk: &media::MediaAssetChunkResponse,
) -> Result<(), &'static str> {
    let error = media::MediaError::try_from(chunk.error).map_err(|_| "media error is invalid")?;
    if error == media::MediaError::Unspecified {
        return Err("media error is invalid");
    }
    (chunk.message.len() <= 256
        && chunk.data.len() <= MEDIA_MAX_CHUNK_BYTES
        && (chunk.transfer_id.is_empty() || valid_media_token(&chunk.transfer_id, 128))
        && (error != media::MediaError::MediaOk
            || (!chunk.transfer_id.is_empty() && (!chunk.data.is_empty() || chunk.eof))))
        .then_some(())
        .ok_or("media asset chunk is invalid")
}

pub fn validate_flight_control_state(
    state: &sunray::FlightControlState,
) -> Result<(), &'static str> {
    (state.battery_voltage_v.is_finite()
        && state.battery_voltage_v >= 0.0
        && state.battery_percent <= 100)
        .then_some(())
        .ok_or("flight control state is invalid")
}

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

fn finite_vector2(value: &mobility::Vector2) -> bool {
    value.x.is_finite() && value.y.is_finite()
}

fn finite_vector3(value: &mobility::Vector3) -> bool {
    value.x.is_finite() && value.y.is_finite() && value.z.is_finite()
}

pub fn validate_uav_direct_control_goal(
    goal: &sunray::UavDirectControlGoal,
) -> Result<(), &'static str> {
    use sunray::uav_direct_control_goal::Target;

    let yaw = goal
        .yaw
        .as_ref()
        .ok_or("yaw target is missing or invalid")?;
    if !(0..=2).contains(&yaw.mode) || !yaw.value.is_finite() {
        return Err("yaw target is missing or invalid");
    }
    if !(0..=2).contains(&goal.controller) {
        return Err("controller is invalid");
    }
    let continuous_lease =
        || (MIN_DIRECT_CONTROL_LEASE_MS..=MAX_DIRECT_CONTROL_LEASE_MS).contains(&goal.lease_ms);
    match goal.target.as_ref() {
        Some(Target::WorldPosition(value))
            if goal.lease_ms == 0
                && !value.frame_id.is_empty()
                && value.position_m.as_ref().is_some_and(finite_vector3) =>
        {
            Ok(())
        }
        Some(Target::WorldPosition(_)) => Err("world position target is invalid"),
        Some(Target::BodyPosition(value))
            if goal.lease_ms == 0
                && value
                    .body_xy_position_m
                    .as_ref()
                    .is_some_and(finite_vector2)
                && value.fixed_height_m.is_finite()
                && value.fixed_height_m > 0.0 =>
        {
            Ok(())
        }
        Some(Target::BodyPosition(_)) => Err("body position target is invalid"),
        Some(Target::TrajectorySetpoint(value))
            if continuous_lease()
                && !value.frame_id.is_empty()
                && value.position_m.as_ref().is_some_and(finite_vector3)
                && value.velocity_mps.as_ref().is_some_and(finite_vector3)
                && value.acceleration_mps2.as_ref().is_some_and(finite_vector3) =>
        {
            Ok(())
        }
        Some(Target::TrajectorySetpoint(_)) => Err("trajectory setpoint target is invalid"),
        Some(Target::WorldVelocity(value))
            if continuous_lease()
                && !value.frame_id.is_empty()
                && value.velocity_mps.as_ref().is_some_and(finite_vector3)
                && value.height_lock.as_ref().map_or(true, |lock| {
                    lock.height_m.is_finite() && lock.height_m > 0.0
                }) =>
        {
            Ok(())
        }
        Some(Target::WorldVelocity(_)) => Err("world velocity target is invalid"),
        Some(Target::BodyVelocity(value))
            if continuous_lease()
                && value
                    .body_xy_velocity_mps
                    .as_ref()
                    .is_some_and(finite_vector2)
                && value.fixed_height_m.is_finite()
                && value.fixed_height_m > 0.0 =>
        {
            Ok(())
        }
        Some(Target::BodyVelocity(_)) => Err("body velocity target is invalid"),
        None => Err("direct control target is missing"),
    }
}

pub fn validate_emergency_kill_goal(goal: &sunray::EmergencyKillGoal) -> Result<(), &'static str> {
    if goal.confirmed {
        Ok(())
    } else {
        Err("emergency kill requires explicit confirmation")
    }
}

pub fn validate_takeoff_goal(goal: &sunray::TakeoffGoal) -> Result<(), &'static str> {
    (goal.takeoff_relative_height_m.is_finite()
        && goal.takeoff_relative_height_m >= 0.0
        && goal.takeoff_max_velocity_mps.is_finite()
        && goal.takeoff_max_velocity_mps >= 0.0)
        .then_some(())
        .ok_or("takeoff goal is invalid")
}

pub fn validate_land_goal(goal: &sunray::LandGoal) -> Result<(), &'static str> {
    (goal.land_max_velocity_mps.is_finite() && goal.land_max_velocity_mps >= 0.0)
        .then_some(())
        .ok_or("land goal is invalid")
}

pub fn validate_uav_waypoint_mission_goal(
    goal: &sunray::UavWaypointMissionGoal,
) -> Result<(), &'static str> {
    if goal.frame_id.is_empty() {
        return Err("waypoint frame is missing");
    }
    if goal.task_name.is_empty() || goal.task_name.len() > MAX_WAYPOINT_TASK_NAME_BYTES {
        return Err("waypoint task name is invalid");
    }
    if !(0..=2).contains(&goal.completion_action) {
        return Err("waypoint completion action is invalid");
    }
    if goal.waypoints.is_empty() || goal.waypoints.len() > MAX_WAYPOINT_COUNT {
        return Err("waypoint count is invalid");
    }
    if goal.waypoints.iter().any(|waypoint| {
        !waypoint.position_m.as_ref().is_some_and(finite_vector3)
            || !waypoint.yaw_rad.is_finite()
            || !waypoint.hold_time_s.is_finite()
            || waypoint.hold_time_s < 0.0
            || !(0..=2).contains(&waypoint.arrival_action)
    }) {
        return Err("waypoint is invalid");
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
