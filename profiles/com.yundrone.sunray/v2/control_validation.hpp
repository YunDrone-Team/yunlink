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
bool validate_uav_direct_control_goal(const UavDirectControlGoal& goal,
                                      std::string* error = nullptr);
bool validate_emergency_kill_goal(const EmergencyKillGoal& goal, std::string* error = nullptr);
bool validate_takeoff_goal(const TakeoffGoal& goal, std::string* error = nullptr);
bool validate_land_goal(const LandGoal& goal, std::string* error = nullptr);
bool validate_uav_waypoint_mission_goal(const UavWaypointMissionGoal& goal,
                                        std::string* error = nullptr);

}  // namespace com::yundrone::sunray::v2

#endif
