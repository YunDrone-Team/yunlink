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
        && state.battery_percent <= 100)
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
