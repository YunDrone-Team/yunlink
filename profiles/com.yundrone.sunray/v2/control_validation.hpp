#ifndef YUNLINK_PROFILE_SUNRAY_CONTROL_VALIDATION_HPP
#define YUNLINK_PROFILE_SUNRAY_CONTROL_VALIDATION_HPP

#include <cstdint>
#include <string>

#include "com.yundrone.sunray/v2/sunray.pb.h"

namespace com::yundrone::sunray::v2 {

constexpr uint32_t kMinDirectControlLeaseMs = 250;
constexpr uint32_t kMaxDirectControlLeaseMs = 2000;
constexpr int kMaxWaypointCount = 256;
constexpr size_t kMaxWaypointTaskNameBytes = 96;

bool validate_flight_control_state(const FlightControlState& state, std::string* error = nullptr);
bool validate_ugv_control_state(const UgvControlState& state, std::string* error = nullptr);
bool validate_uav_direct_control_goal(const UavDirectControlGoal& goal,
                                      std::string* error = nullptr);
bool validate_emergency_kill_goal(const EmergencyKillGoal& goal, std::string* error = nullptr);
bool validate_takeoff_goal(const TakeoffGoal& goal, std::string* error = nullptr);
bool validate_land_goal(const LandGoal& goal, std::string* error = nullptr);
bool validate_uav_waypoint_mission_goal(const UavWaypointMissionGoal& goal,
                                        std::string* error = nullptr);
bool validate_uav_nav_goal(const UavNavGoal& goal, std::string* error = nullptr);
bool validate_ugv_move_point_goal(const UgvMovePointGoal& goal, std::string* error = nullptr);
bool validate_ugv_velocity_goal(const UgvVelocityGoal& goal, std::string* error = nullptr);
bool validate_ugv_nav_goal(const UgvNavGoal& goal, std::string* error = nullptr);
bool validate_ugv_waypoint_mission_goal(const UgvWaypointMissionGoal& goal,
                                        std::string* error = nullptr);
bool validate_ugv_planning_state(const UgvPlanningState& state, std::string* error = nullptr);
bool validate_planner_set_home_request(const PlannerSetHomeRequest& request,
                                       std::string* error = nullptr);
bool validate_formation_set_request(const FormationSetRequest& request,
                                    std::string* error = nullptr);
bool validate_formation_leader_target_request(const FormationLeaderTargetRequest& request,
                                              std::string* error = nullptr);
bool validate_formation_state(const FormationState& state, std::string* error = nullptr);
bool validate_gimbal_angle_goal(const GimbalAngleGoal& goal, std::string* error = nullptr);
bool validate_gimbal_rate_goal(const GimbalRateGoal& goal, std::string* error = nullptr);
bool validate_gimbal_zoom_absolute_goal(const GimbalZoomAbsoluteGoal& goal,
                                        std::string* error = nullptr);

}  // namespace com::yundrone::sunray::v2

#endif
