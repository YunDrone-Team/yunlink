use crate::{mobility, sunray};
use std::collections::HashSet;

use super::validation_sunray::finite_vector3;

fn finite_pose(value: &mobility::Pose) -> bool {
    let Some(position) = value.position.as_ref() else {
        return false;
    };
    let Some(orientation) = value.orientation.as_ref() else {
        return false;
    };
    let norm_squared = orientation.x * orientation.x
        + orientation.y * orientation.y
        + orientation.z * orientation.z
        + orientation.w * orientation.w;
    finite_vector3(position) && norm_squared.is_finite() && norm_squared > 1e-12
}

fn double_ring(
    request: &sunray::FormationSetRequest,
    dynamic: bool,
) -> Result<(), &'static str> {
    let value = request
        .double_ring
        .as_ref()
        .ok_or("formation double ring is invalid")?;
    if !(value.radius_m.is_finite() && value.radius_m > 0.0)
        || !value.lower_height_m.is_finite()
        || !value.upper_height_m.is_finite()
        || !(value.lower_height_m < value.upper_height_m)
        || !value.phase_offset_rad.is_finite()
    {
        return Err("formation double ring geometry is invalid");
    }
    // STATIC_DOUBLE_RING ignores angular speed entirely, so an unused non-finite
    // value stays acceptable; the dynamic variant requires finite non-zero motion.
    if dynamic && !(value.angular_speed_radps.is_finite() && value.angular_speed_radps.abs() > 0.0)
    {
        return Err("dynamic formation double ring angular speed is invalid");
    }
    Ok(())
}

pub fn validate_formation_set_request(
    request: &sunray::FormationSetRequest,
) -> Result<(), &'static str> {
    let positive = |value: f64| value.is_finite() && value > 0.0;
    let moving = |value: f64| value.is_finite() && value.abs() > 0.0;
    // Height semantics are part of the task contract: an unsupported mode is
    // rejected for every formation type, even one that ignores planar height.
    if request.height_mode != 0 && request.height_mode != 1 {
        return Err("formation height mode is invalid");
    }
    if !request.height_m.is_finite() {
        return Err("formation height is invalid");
    }
    match request.formation_type {
        1 | 2 => {
            if request.height_mode == 0 && request.height_m == 0.0 {
                Ok(())
            } else {
                Err("takeoff and land require legacy height mode and zero height")
            }
        }
        10 => request
            .line
            .as_ref()
            .filter(|value| positive(value.spacing_m) && value.angle_deg.is_finite())
            .map(|_| ())
            .ok_or("formation line is invalid"),
        11 => request
            .polygon
            .as_ref()
            .filter(|value| positive(value.side_length_m))
            .map(|_| ())
            .ok_or("formation polygon is invalid"),
        20 => request
            .polygon
            .as_ref()
            .filter(|value| positive(value.side_length_m) && moving(value.move_speed_mps))
            .map(|_| ())
            .ok_or("dynamic formation polygon is invalid"),
        21 => request
            .ring
            .as_ref()
            .filter(|value| positive(value.radius_m) && moving(value.move_speed_mps))
            .map(|_| ())
            .ok_or("dynamic formation ring is invalid"),
        22 => request
            .lemniscate
            .as_ref()
            .filter(|value| {
                positive(value.x_scale_m)
                    && positive(value.y_scale_m)
                    && moving(value.move_speed_mps)
            })
            .map(|_| ())
            .ok_or("dynamic formation lemniscate is invalid"),
        13 => double_ring(request, false),
        23 => double_ring(request, true),
        12 => {
            let leader = request
                .leader
                .as_ref()
                .ok_or("formation leader layout is invalid")?;
            if leader.agent_slots.len() != 25
                || leader.virtual_leader_slots.len() != 25
                || !positive(leader.spacing_m)
            {
                return Err("formation leader layout is invalid");
            }
            let agents: Vec<_> = leader
                .agent_slots
                .iter()
                .copied()
                .filter(|slot| *slot != 0)
                .collect();
            if agents.is_empty()
                || agents.iter().any(|slot| *slot > 255)
                || agents.iter().copied().collect::<HashSet<_>>().len() != agents.len()
            {
                return Err("formation leader agent slots are invalid");
            }
            (leader
                .virtual_leader_slots
                .iter()
                .filter(|slot| **slot)
                .count()
                == 1)
                .then_some(())
                .ok_or("formation leader target slot is invalid")
        }
        _ => Err("formation type is invalid"),
    }
}

pub fn validate_formation_leader_target_request(
    request: &sunray::FormationLeaderTargetRequest,
) -> Result<(), &'static str> {
    match request.target_mode {
        1 if !request.frame_id.is_empty()
            && request.target_pose.as_ref().is_some_and(finite_pose) =>
        {
            Ok(())
        }
        1 => Err("formation leader fixed pose is invalid"),
        2 if request.odom_topic.len() > 1 && request.odom_topic.starts_with('/') => Ok(()),
        2 => Err("formation leader odometry topic is invalid"),
        _ => Err("formation leader target mode is invalid"),
    }
}

pub fn validate_formation_state(state: &sunray::FormationState) -> Result<(), &'static str> {
    let valid_type = matches!(
        state.formation_type,
        0 | 1 | 2 | 10 | 11 | 12 | 13 | 20 | 21 | 22 | 23
    );
    let valid_target = !state.virtual_leader_target_valid
        || state
            .virtual_leader_target
            .as_ref()
            .is_some_and(finite_pose);
    ((0..=4).contains(&state.phase)
        && (0..=4).contains(&state.dynamic_start_status)
        && valid_type
        && valid_target)
        .then_some(())
        .ok_or("formation state is invalid")
}

pub fn validate_mapping_state(state: &sunray::MappingState) -> Result<(), &'static str> {
    let valid_status = matches!(
        state.status.as_str(),
        "" | "UNAVAILABLE" | "IDLE" | "ACCUMULATING" | "ERROR"
    );
    let valid_lidars = state.lidars.iter().all(|lidar| {
        !lidar.lidar_rate_hz.is_nan()
            && !lidar.imu_rate_hz.is_nan()
            && !lidar.lidar_age_sec.is_nan()
            && !lidar.imu_age_sec.is_nan()
    });
    (valid_status && valid_lidars)
        .then_some(())
        .ok_or("mapping state is invalid")
}

pub fn validate_gimbal_angle_goal(goal: &sunray::GimbalAngleGoal) -> Result<(), &'static str> {
    (goal.yaw_rad.is_finite() && goal.pitch_rad.is_finite())
        .then_some(())
        .ok_or("gimbal angle goal is invalid")
}

pub fn validate_gimbal_rate_goal(goal: &sunray::GimbalRateGoal) -> Result<(), &'static str> {
    ((-100..=100).contains(&goal.yaw_control) && (-100..=100).contains(&goal.pitch_control))
        .then_some(())
        .ok_or("gimbal rate control must be between -100 and 100")
}

pub fn validate_gimbal_zoom_absolute_goal(
    goal: &sunray::GimbalZoomAbsoluteGoal,
) -> Result<(), &'static str> {
    (goal.zoom.is_finite() && (1.0..=30.9).contains(&goal.zoom))
        .then_some(())
        .ok_or("gimbal zoom must be between 1.0 and 30.9")
}
