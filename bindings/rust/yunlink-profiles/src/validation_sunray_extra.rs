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

pub fn validate_formation_set_request(
    request: &sunray::FormationSetRequest,
) -> Result<(), &'static str> {
    let positive = |value: f64| value.is_finite() && value > 0.0;
    let moving = |value: f64| value.is_finite() && value.abs() > 0.0;
    match request.formation_type {
        1 | 2 => Ok(()),
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
        // 13/23 were removed; the wire values stay reserved and are rejected with a
        // readable reason instead of falling through to the generic invalid-type arm.
        13 | 23 => Err("double-ring formations were removed"),
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

/// 2.17 小队目标：与 formation leader target 是两条独立 action。
/// team_id 必填非空是**安全契约**（没有「空 = 全部小队」），空值一律在协议层拒绝。
pub fn validate_team_goal_request(
    request: &sunray::TeamGoalRequest,
) -> Result<(), &'static str> {
    if request.team_id.is_empty() {
        return Err("team goal request team_id must not be empty");
    }
    if request.frame_id.is_empty() {
        return Err("team goal request frame_id must not be empty");
    }
    if !request.target_pose.as_ref().is_some_and(finite_pose) {
        return Err("team goal target pose is invalid");
    }
    Ok(())
}

pub fn validate_formation_state(state: &sunray::FormationState) -> Result<(), &'static str> {
    // 13/23 are reserved and no longer valid echo types: an old peer's state frame
    // is treated as unknown/unsupported (dropped for diagnosis, never rendered).
    let valid_type = matches!(
        state.formation_type,
        0 | 1 | 2 | 10 | 11 | 12 | 20 | 21 | 22
    );
    let valid_target = !state.virtual_leader_target_valid
        || state
            .virtual_leader_target
            .as_ref()
            .is_some_and(finite_pose);
    ((0..=4).contains(&state.phase)
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

// ---------------------------------------------------------------------------
// 2.21 swarm 级 Team 目标（先预览选位，再确认下发）。
//
// 这里只做**协议层**校验：坏值在网关就拦下并给出具体原因，不必等 ROS 服务超时。
// 业务下限（safety_min_spacing_m，当前 0.6）来自机侧响应，**不在这里硬编码**；
// 可达性 / 路径 / 轨迹可行性一律不判（回调优化无法预测，用户已明确否决）。
// ---------------------------------------------------------------------------

/// 协调器 `global_frame` 默认值；写错会被直接拒（INVALID_COMMON_TASK）。
pub const SWARM_TEAM_GOAL_FRAME_ID: &str = "world";

const SWARM_TEAM_GOAL_LAYOUTS: [&str; 5] = ["", "optimized", "circle", "triangle", "line"];

pub fn validate_swarm_team_goal_request(
    request: &sunray::SwarmTeamGoalRequest,
) -> Result<(), &'static str> {
    if request.request_id == 0 {
        return Err("swarm team goal request_id must be non-zero");
    }
    let Some(target_pose) = request.target_pose.as_ref() else {
        return Err("swarm team goal target_pose is missing");
    };
    if target_pose.frame_id != SWARM_TEAM_GOAL_FRAME_ID {
        return Err("swarm team goal frame_id must be world");
    }
    if !target_pose.pose.as_ref().is_some_and(finite_pose) {
        return Err("swarm team goal target pose is invalid (non-zero orientation required)");
    }
    if !SWARM_TEAM_GOAL_LAYOUTS.contains(&request.layout.as_str()) {
        return Err("swarm team goal layout is invalid");
    }
    if !request.desired_spacing_m.is_finite() || request.desired_spacing_m < 0.0 {
        return Err("swarm team goal desired_spacing_m must be finite and non-negative");
    }
    Ok(())
}

/// `team_ids[i] ↔ targets[i]` 是协调器的硬承诺；长度不等说明桥转发出错，
/// 必须当成坏响应而不是"少画几个点"（那样操作员会照着不完整的预览去确认）。
pub fn validate_swarm_team_goal_response(
    response: &sunray::SwarmTeamGoalResponse,
) -> Result<(), &'static str> {
    if response.team_ids.len() != response.targets.len() {
        return Err("swarm team goal team_ids/targets length mismatch");
    }
    for target in &response.targets {
        let finite_xy = target
            .pose
            .as_ref()
            .and_then(|pose| pose.position.as_ref())
            .is_some_and(|position| position.x.is_finite() && position.y.is_finite());
        if !finite_xy {
            return Err("swarm team goal target position is not finite");
        }
    }
    if !response.actual_min_spacing_m.is_finite() || !response.safety_min_spacing_m.is_finite() {
        return Err("swarm team goal spacing readouts are not finite");
    }
    Ok(())
}

#[cfg(test)]
mod swarm_team_goal_validation_tests {
    use super::*;

    fn valid_request() -> sunray::SwarmTeamGoalRequest {
        sunray::SwarmTeamGoalRequest {
            target_pose: Some(sunray::SwarmTeamGoalPoseStamped {
                frame_id: "world".into(),
                pose: Some(mobility::Pose {
                    position: Some(mobility::Vector3 { x: -1.0, y: 0.2, z: 0.0 }),
                    orientation: Some(mobility::Quaternion { x: 0.0, y: 0.0, z: 0.0, w: 1.0 }),
                }),
            }),
            layout: "triangle".into(),
            request_id: 1,
            preview_only: true,
            desired_spacing_m: 0.0,
        }
    }

    #[test]
    fn accepts_a_valid_preview_request() {
        assert!(validate_swarm_team_goal_request(&valid_request()).is_ok());
        let mut auto = valid_request();
        auto.layout.clear();
        assert!(validate_swarm_team_goal_request(&auto).is_ok(), "空 layout = optimized");
    }

    #[test]
    fn rejects_zero_id_wrong_frame_zero_quaternion_bad_layout_and_negative_spacing() {
        let mut request = valid_request();
        request.request_id = 0;
        assert_eq!(validate_swarm_team_goal_request(&request),
            Err("swarm team goal request_id must be non-zero"));

        request = valid_request();
        request.target_pose.as_mut().unwrap().frame_id = "map".into();
        assert_eq!(validate_swarm_team_goal_request(&request),
            Err("swarm team goal frame_id must be world"));

        request = valid_request();
        request.target_pose.as_mut().unwrap().pose.as_mut().unwrap().orientation =
            Some(mobility::Quaternion { x: 0.0, y: 0.0, z: 0.0, w: 0.0 });
        assert_eq!(validate_swarm_team_goal_request(&request),
            Err("swarm team goal target pose is invalid (non-zero orientation required)"));

        request = valid_request();
        request.layout = "hexagon".into();
        assert_eq!(validate_swarm_team_goal_request(&request),
            Err("swarm team goal layout is invalid"));

        request = valid_request();
        request.desired_spacing_m = -0.1;
        assert_eq!(validate_swarm_team_goal_request(&request),
            Err("swarm team goal desired_spacing_m must be finite and non-negative"));
    }

    #[test]
    fn response_requires_aligned_team_ids_and_targets() {
        let mut response = sunray::SwarmTeamGoalResponse::default();
        response.team_ids.push("team1".into());
        assert_eq!(validate_swarm_team_goal_response(&response),
            Err("swarm team goal team_ids/targets length mismatch"));
        response.targets.push(sunray::SwarmTeamGoalPoseStamped {
            frame_id: "world".into(),
            pose: Some(mobility::Pose {
                position: Some(mobility::Vector3 { x: 1.0, y: 0.0, z: 0.8 }),
                orientation: Some(mobility::Quaternion { x: 0.0, y: 0.0, z: 0.0, w: 1.0 }),
            }),
        });
        assert!(validate_swarm_team_goal_response(&response).is_ok());
    }
}
