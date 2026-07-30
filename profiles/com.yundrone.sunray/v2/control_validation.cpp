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
        if (!valid_continuous_lease(goal.lease_ms()) ||
            goal.world_velocity().frame_id().empty() ||
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

bool validate_uav_waypoint_mission_goal(const UavWaypointMissionGoal& goal,
                                        std::string* error) {
    if (goal.frame_id().empty()) {
        return fail(error, "waypoint frame is missing");
    }
    if (goal.waypoints_size() == 0 || goal.waypoints_size() > kMaxWaypointCount) {
        return fail(error, "waypoint count is invalid");
    }
    for (const auto& waypoint : goal.waypoints()) {
        if (!waypoint.has_position_m() || !finite(waypoint.position_m()) ||
            !std::isfinite(waypoint.yaw_rad()) || !std::isfinite(waypoint.hold_time_s()) ||
            waypoint.hold_time_s() < 0.0) {
            return fail(error, "waypoint is invalid");
        }
    }
    return true;
}

}  // namespace com::yundrone::sunray::v2
