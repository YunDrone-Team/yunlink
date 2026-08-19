use crate::{
    mobility, sunray, MAX_DIRECT_CONTROL_LEASE_MS, MAX_WAYPOINT_COUNT,
    MAX_WAYPOINT_TASK_NAME_BYTES, MIN_DIRECT_CONTROL_LEASE_MS,
};

fn finite_vector2(value: &mobility::Vector2) -> bool {
    value.x.is_finite() && value.y.is_finite()
}

fn finite_vector3(value: &mobility::Vector3) -> bool {
    value.x.is_finite() && value.y.is_finite() && value.z.is_finite()
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
