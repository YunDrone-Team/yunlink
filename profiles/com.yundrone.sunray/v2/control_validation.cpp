#include "com.yundrone.sunray/v2/control_validation.hpp"

#include <cmath>

namespace com::yundrone::sunray::v2 {
namespace {

bool fail(std::string* error, const char* detail) {
    if (error != nullptr) {
        *error = detail;
    }
    return false;
}

bool finite(const org::yunlink::mobility::v1::Vector2& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y());
}

bool finite(const org::yunlink::mobility::v1::Vector3& value) {
    return std::isfinite(value.x()) && std::isfinite(value.y()) && std::isfinite(value.z());
}

bool valid_yaw(const UavDirectControlGoal& goal) {
    if (!goal.has_yaw() || !std::isfinite(goal.yaw().value())) {
        return false;
    }
    return goal.yaw().mode() == UAV_YAW_KEEP || goal.yaw().mode() == UAV_YAW_SET_ANGLE ||
           goal.yaw().mode() == UAV_YAW_SET_RATE;
}

bool valid_controller(const UavDirectControlGoal& goal) {
    return goal.controller() == UAV_CONTROLLER_DEFAULT ||
           goal.controller() == UAV_CONTROLLER_POSITION ||
           goal.controller() == UAV_CONTROLLER_ATTITUDE;
}

bool valid_continuous_lease(uint32_t lease_ms) {
    return lease_ms >= kMinDirectControlLeaseMs && lease_ms <= kMaxDirectControlLeaseMs;
}

}  // namespace

bool validate_flight_control_state(const FlightControlState& state, std::string* error) {
    if (!std::isfinite(state.battery_voltage_v()) || state.battery_voltage_v() < 0.0F ||
        state.battery_percent() > 100U) {
        return fail(error, "flight control state is invalid");
    }
    return true;
}

bool validate_uav_direct_control_goal(const UavDirectControlGoal& goal, std::string* error) {
    if (!valid_yaw(goal)) {
        return fail(error, "yaw target is missing or invalid");
    }
    if (!valid_controller(goal)) {
        return fail(error, "controller is invalid");
    }
    switch (goal.target_case()) {
    case UavDirectControlGoal::kWorldPosition:
        if (goal.lease_ms() != 0 || goal.world_position().frame_id().empty() ||
            !goal.world_position().has_position_m() ||
            !finite(goal.world_position().position_m())) {
            return fail(error, "world position target is invalid");
        }
        return true;
    case UavDirectControlGoal::kBodyPosition:
        if (goal.lease_ms() != 0 || !goal.body_position().has_body_xy_position_m() ||
            !finite(goal.body_position().body_xy_position_m()) ||
            !std::isfinite(goal.body_position().fixed_height_m()) ||
            goal.body_position().fixed_height_m() <= 0.0) {
            return fail(error, "body position target is invalid");
        }
        return true;
    case UavDirectControlGoal::kTrajectorySetpoint:
        if (!valid_continuous_lease(goal.lease_ms()) ||
            goal.trajectory_setpoint().frame_id().empty() ||
            !goal.trajectory_setpoint().has_position_m() ||
            !goal.trajectory_setpoint().has_velocity_mps() ||
            !goal.trajectory_setpoint().has_acceleration_mps2() ||
            !finite(goal.trajectory_setpoint().position_m()) ||
            !finite(goal.trajectory_setpoint().velocity_mps()) ||
            !finite(goal.trajectory_setpoint().acceleration_mps2())) {
            return fail(error, "trajectory setpoint target is invalid");
        }
        return true;
    case UavDirectControlGoal::kWorldVelocity:
        if (!valid_continuous_lease(goal.lease_ms()) || goal.world_velocity().frame_id().empty() ||
            !goal.world_velocity().has_velocity_mps() ||
            !finite(goal.world_velocity().velocity_mps()) ||
            (goal.world_velocity().has_height_lock() &&
             (!std::isfinite(goal.world_velocity().height_lock().height_m()) ||
              goal.world_velocity().height_lock().height_m() <= 0.0))) {
            return fail(error, "world velocity target is invalid");
        }
        return true;
    case UavDirectControlGoal::kBodyVelocity:
        if (!valid_continuous_lease(goal.lease_ms()) ||
            !goal.body_velocity().has_body_xy_velocity_mps() ||
            !finite(goal.body_velocity().body_xy_velocity_mps()) ||
            !std::isfinite(goal.body_velocity().fixed_height_m()) ||
            goal.body_velocity().fixed_height_m() <= 0.0) {
            return fail(error, "body velocity target is invalid");
        }
        return true;
    case UavDirectControlGoal::TARGET_NOT_SET:
        return fail(error, "direct control target is missing");
    }
    return fail(error, "direct control target is invalid");
}

bool validate_emergency_kill_goal(const EmergencyKillGoal& goal, std::string* error) {
    return goal.confirmed() || fail(error, "emergency kill requires explicit confirmation");
}

bool validate_takeoff_goal(const TakeoffGoal& goal, std::string* error) {
    if (!std::isfinite(goal.takeoff_relative_height_m()) ||
        goal.takeoff_relative_height_m() < 0.0 || !std::isfinite(goal.takeoff_max_velocity_mps()) ||
        goal.takeoff_max_velocity_mps() < 0.0) {
        return fail(error, "takeoff goal is invalid");
    }
    return true;
}

bool validate_land_goal(const LandGoal& goal, std::string* error) {
    return std::isfinite(goal.land_max_velocity_mps()) && goal.land_max_velocity_mps() >= 0.0 ||
           fail(error, "land goal is invalid");
}

bool validate_uav_waypoint_mission_goal(const UavWaypointMissionGoal& goal, std::string* error) {
    if (goal.frame_id().empty()) {
        return fail(error, "waypoint frame is missing");
    }
    if (goal.task_name().empty() || goal.task_name().size() > kMaxWaypointTaskNameBytes) {
        return fail(error, "waypoint task name is invalid");
    }
    if (goal.completion_action() != UAV_MISSION_FINISH_HOVER &&
        goal.completion_action() != UAV_MISSION_FINISH_RETURN_HOME_AND_LAND &&
        goal.completion_action() != UAV_MISSION_FINISH_LAND_NOW) {
        return fail(error, "waypoint completion action is invalid");
    }
    if (goal.waypoints_size() == 0 || goal.waypoints_size() > kMaxWaypointCount) {
        return fail(error, "waypoint count is invalid");
    }
    for (const auto& waypoint : goal.waypoints()) {
        const bool holding = waypoint.arrival_action() == UAV_WAYPOINT_HOLD_CURRENT_YAW ||
                             waypoint.arrival_action() == UAV_WAYPOINT_HOLD_SET_YAW;
        if (!waypoint.has_position_m() || !finite(waypoint.position_m()) ||
            !std::isfinite(waypoint.yaw_rad()) || !std::isfinite(waypoint.hold_time_s()) ||
            waypoint.hold_time_s() < 0.0 ||
            (waypoint.arrival_action() != UAV_WAYPOINT_NEXT && !holding)) {
            return fail(error, "waypoint is invalid");
        }
    }
    return true;
}

bool validate_uav_nav_goal(const UavNavGoal& goal, std::string* error) {
    if (goal.frame_id().empty() || !goal.has_position_m() || !finite(goal.position_m()) ||
        !std::isfinite(goal.yaw_rad())) {
        return fail(error, "UAV navigation goal is invalid");
    }
    return true;
}

bool validate_ugv_move_point_goal(const UgvMovePointGoal& goal, std::string* error) {
    if (!goal.has_point_m() || !finite(goal.point_m()) || goal.point_m().z() != 0.0 ||
        !std::isfinite(goal.desired_yaw_rad())) {
        return fail(error, "UGV move point goal is invalid");
    }
    if (goal.frame() != UGV_MOVE_LOCAL && goal.frame() != UGV_MOVE_BODY) {
        return fail(error, "UGV move point frame is invalid");
    }
    if (goal.yaw_mode() != UGV_YAW_KEEP && goal.yaw_mode() != UGV_YAW_SET) {
        return fail(error, "UGV yaw mode is invalid");
    }
    if ((goal.frame() == UGV_MOVE_LOCAL) != !goal.local_frame_id().empty()) {
        return fail(error, "UGV local frame contract is invalid");
    }
    return true;
}

bool validate_ugv_velocity_goal(const UgvVelocityGoal& goal, std::string* error) {
    if (!valid_continuous_lease(goal.lease_ms())) {
        return fail(error, "UGV velocity lease is invalid");
    }
    switch (goal.target_case()) {
    case UgvVelocityGoal::kLocal:
        if (goal.local().frame_id().empty() || !goal.local().has_linear_mps() ||
            !finite(goal.local().linear_mps()) || !std::isfinite(goal.local().desired_yaw_rad())) {
            return fail(error, "UGV local velocity target is invalid");
        }
        return true;
    case UgvVelocityGoal::kBody:
        if (!goal.body().has_linear_mps() || !finite(goal.body().linear_mps()) ||
            !std::isfinite(goal.body().yaw_rate_radps())) {
            return fail(error, "UGV body velocity target is invalid");
        }
        return true;
    case UgvVelocityGoal::TARGET_NOT_SET:
        return fail(error, "UGV velocity target is missing");
    }
    return fail(error, "UGV velocity target is invalid");
}

bool validate_planner_set_home_request(const PlannerSetHomeRequest& request, std::string* error) {
    if (request.frame_id().empty() || !request.has_home_m() || !finite(request.home_m())) {
        return fail(error, "Planner home request is invalid");
    }
    return true;
}

bool validate_gimbal_angle_goal(const GimbalAngleGoal& goal, std::string* error) {
    if (!std::isfinite(goal.yaw_rad()) || !std::isfinite(goal.pitch_rad())) {
        return fail(error, "gimbal angle goal is invalid");
    }
    return true;
}

bool validate_gimbal_rate_goal(const GimbalRateGoal& goal, std::string* error) {
    if (goal.yaw_control() < -100 || goal.yaw_control() > 100 ||
        goal.pitch_control() < -100 || goal.pitch_control() > 100) {
        return fail(error, "gimbal rate control must be between -100 and 100");
    }
    return true;
}

bool validate_gimbal_zoom_absolute_goal(const GimbalZoomAbsoluteGoal& goal, std::string* error) {
    if (!std::isfinite(goal.zoom()) || goal.zoom() < 1.0 || goal.zoom() > 30.9) {
        return fail(error, "gimbal zoom must be between 1.0 and 30.9");
    }
    return true;
}

}  // namespace com::yundrone::sunray::v2
