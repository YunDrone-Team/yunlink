use crate::{
    mobility, sunray, MAX_DIRECT_CONTROL_LEASE_MS, MAX_WAYPOINT_COUNT,
    MAX_WAYPOINT_TASK_NAME_BYTES, MIN_DIRECT_CONTROL_LEASE_MS,
};
use std::collections::HashSet;

fn finite_vector2(value: &mobility::Vector2) -> bool {
    value.x.is_finite() && value.y.is_finite()
}

fn finite_vector3(value: &mobility::Vector3) -> bool {
    value.x.is_finite() && value.y.is_finite() && value.z.is_finite()
}

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

pub fn validate_flight_control_state(
    state: &sunray::FlightControlState,
) -> Result<(), &'static str> {
    (state.battery_voltage_v.is_finite()
        && state.battery_voltage_v >= 0.0
        && state.battery_percent <= 100
        && (0..=4).contains(&state.controller_type))
    .then_some(())
    .ok_or("flight control state is invalid")
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

pub fn validate_uav_nav_goal(goal: &sunray::UavNavGoal) -> Result<(), &'static str> {
    (!goal.frame_id.is_empty()
        && goal.position_m.as_ref().is_some_and(finite_vector3)
        && goal.yaw_rad.is_finite())
    .then_some(())
    .ok_or("UAV navigation goal is invalid")
}

pub fn validate_ugv_move_point_goal(goal: &sunray::UgvMovePointGoal) -> Result<(), &'static str> {
    let valid_frame = matches!(goal.frame, 0 | 1);
    let valid_yaw_mode = matches!(goal.yaw_mode, 0 | 1);
    let valid_point = goal
        .point_m
        .as_ref()
        .is_some_and(|point| finite_vector3(point) && point.z == 0.0);
    let valid_frame_id = (goal.frame == 0) == !goal.local_frame_id.is_empty();
    (valid_frame
        && valid_yaw_mode
        && valid_point
        && goal.desired_yaw_rad.is_finite()
        && valid_frame_id)
        .then_some(())
        .ok_or("UGV move point goal is invalid")
}

pub fn validate_ugv_velocity_goal(goal: &sunray::UgvVelocityGoal) -> Result<(), &'static str> {
    use sunray::ugv_velocity_goal::Target;

    if !(MIN_DIRECT_CONTROL_LEASE_MS..=MAX_DIRECT_CONTROL_LEASE_MS).contains(&goal.lease_ms) {
        return Err("UGV velocity lease is invalid");
    }
    match goal.target.as_ref() {
        Some(Target::Local(value))
            if !value.frame_id.is_empty()
                && value.linear_mps.as_ref().is_some_and(finite_vector2)
                && value.desired_yaw_rad.is_finite() =>
        {
            Ok(())
        }
        Some(Target::Local(_)) => Err("UGV local velocity target is invalid"),
        Some(Target::Body(value))
            if value.linear_mps.as_ref().is_some_and(finite_vector2)
                && value.yaw_rate_radps.is_finite() =>
        {
            Ok(())
        }
        Some(Target::Body(_)) => Err("UGV body velocity target is invalid"),
        None => Err("UGV velocity target is missing"),
    }
}

pub fn validate_ugv_nav_goal(goal: &sunray::UgvNavGoal) -> Result<(), &'static str> {
    (!goal.frame_id.is_empty()
        && goal.position_m.as_ref().is_some_and(finite_vector2)
        && goal.yaw_rad.is_finite())
    .then_some(())
    .ok_or("UGV navigation goal is invalid")
}

pub fn validate_ugv_waypoint_mission_goal(
    goal: &sunray::UgvWaypointMissionGoal,
) -> Result<(), &'static str> {
    if goal.frame_id.is_empty() {
        return Err("UGV waypoint frame is missing");
    }
    if goal.task_name.is_empty() || goal.task_name.len() > MAX_WAYPOINT_TASK_NAME_BYTES {
        return Err("UGV waypoint task name is invalid");
    }
    if !matches!(goal.completion_action, 0 | 1) {
        return Err("UGV waypoint completion action is invalid");
    }
    if goal.waypoints.is_empty() || goal.waypoints.len() > MAX_WAYPOINT_COUNT {
        return Err("UGV waypoint count is invalid");
    }
    if goal.waypoints.iter().any(|waypoint| {
        !waypoint.position_m.as_ref().is_some_and(finite_vector2)
            || !waypoint.yaw_rad.is_finite()
            || !waypoint.hold_time_s.is_finite()
            || waypoint.hold_time_s < 0.0
            || !matches!(waypoint.arrival_action, 0..=2)
    }) {
        return Err("UGV waypoint is invalid");
    }
    Ok(())
}

pub fn validate_ugv_planning_state(state: &sunray::UgvPlanningState) -> Result<(), &'static str> {
    let current_valid = state.current_waypoint.as_ref().is_none_or(|waypoint| {
        waypoint.position_m.as_ref().is_some_and(finite_vector2)
            && waypoint.yaw_rad.is_finite()
            && waypoint.hold_time_s.is_finite()
    });
    (state.main_state <= 3
        && state.task_state <= 5
        && state.current_waypoint_index <= state.total_waypoints
        && state.distance_to_goal_m.is_finite()
        && state.distance_to_goal_m >= 0.0
        && state.hold_remaining_s.is_finite()
        && state.hold_remaining_s >= 0.0
        && current_valid)
        .then_some(())
        .ok_or("UGV planning state is invalid")
}

pub fn validate_planner_set_home_request(
    request: &sunray::PlannerSetHomeRequest,
) -> Result<(), &'static str> {
    (!request.frame_id.is_empty() && request.home_m.as_ref().is_some_and(finite_vector3))
        .then_some(())
        .ok_or("Planner home request is invalid")
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
        0 | 1 | 2 | 10 | 11 | 12 | 20 | 21 | 22
    );
    let valid_target = !state.virtual_leader_target_valid
        || state
            .virtual_leader_target
            .as_ref()
            .is_some_and(finite_pose);
    ((0..=4).contains(&state.phase) && valid_type && valid_target)
        .then_some(())
        .ok_or("formation state is invalid")
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
