use crate::{telemetry, SUMMARY_MAX_METRICS, SUMMARY_MAX_PAYLOAD_BYTES};

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
