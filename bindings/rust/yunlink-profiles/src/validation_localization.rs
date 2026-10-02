use crate::{mobility, sunray};

fn vector_valid(v: &mobility::Vector3) -> bool {
    v.x.is_finite() && v.y.is_finite() && v.z.is_finite()
}

fn pose_valid(p: &mobility::Pose) -> bool {
    p.position.as_ref().is_some_and(vector_valid)
        && p.orientation.as_ref().is_some_and(|q| {
            let norm = q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w;
            norm.is_finite() && (norm - 1.0).abs() <= 1e-5
        })
}

/// Invalid states may carry the last trusted pose, but never malformed data.
pub fn validate_localization_state(s: &sunray::LocalizationState) -> Result<(), &'static str> {
    if !(0..=5).contains(&s.status)
        || !s.input_rate_hz.is_finite()
        || s.input_rate_hz < 0.0
        || s.reason.len() > 512
        || s.localization_epoch.len() > 128
    {
        return Err("localization metadata is invalid");
    }
    if let Some(o) = &s.local_odometry {
        if s.localization_epoch.is_empty()
            || s.source_sequence == 0
            || o.frame_id.is_empty()
            || o.child_frame_id.is_empty()
            || s.velocity_frame_id.is_empty()
            || s.velocity_frame_id != o.frame_id
            || !o.pose.as_ref().is_some_and(pose_valid)
            || !o.twist.as_ref().is_some_and(|t| {
                t.linear.as_ref().is_some_and(vector_valid)
                    && t.angular.as_ref().is_some_and(vector_valid)
            })
            || o.pose_covariance
                .iter()
                .chain(&o.twist_covariance)
                .any(|v| !v.is_finite())
        {
            return Err("localization pose is invalid");
        }
    }
    if s.scene_transform_valid
        && (s.scene_frame_id.is_empty() || !s.scene_from_local.as_ref().is_some_and(pose_valid))
    {
        return Err("localization scene transform is invalid");
    }
    if s.status == sunray::LocalizationStatus::LocalizationValid as i32
        && (s.local_odometry.is_none() || !s.scene_transform_valid)
    {
        return Err("valid localization requires pose and scene transform");
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use prost::Message;

    #[test]
    fn absent_source_golden_and_unknown_fields() {
        let waiting = sunray::LocalizationState::default();
        assert_eq!(waiting.encode_to_vec(), Vec::<u8>::new());
        let missing = sunray::LocalizationState {
            status: 2,
            reason: "missing".into(),
            ..Default::default()
        };
        let golden = b"\x38\x02\x42\x07missing";
        assert_eq!(missing.encode_to_vec(), golden);
        assert_eq!(
            sunray::LocalizationState::decode(golden.as_slice()).unwrap(),
            missing
        );
        let mut extended = golden.to_vec();
        extended.extend_from_slice(b"\xa0\x06\x01");
        assert_eq!(
            sunray::LocalizationState::decode(extended.as_slice()).unwrap(),
            missing
        );
        assert!(validate_localization_state(&missing).is_ok());
    }

    #[test]
    fn valid_requires_atomic_pose_transform_and_finite_data() {
        let mut s = sunray::LocalizationState {
            status: 1,
            ..Default::default()
        };
        assert!(validate_localization_state(&s).is_err());
        s.status = 3;
        assert!(validate_localization_state(&s).is_ok());
        s.input_rate_hz = f64::NAN;
        assert!(validate_localization_state(&s).is_err());
        s.input_rate_hz = 10.0;
        s.scene_transform_valid = true;
        assert!(validate_localization_state(&s).is_err());
    }
}
