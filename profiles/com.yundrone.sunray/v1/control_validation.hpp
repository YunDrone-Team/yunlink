#ifndef YUNLINK_PROFILE_SUNRAY_CONTROL_VALIDATION_HPP
#define YUNLINK_PROFILE_SUNRAY_CONTROL_VALIDATION_HPP

#include <cstdint>
#include <string>

#include "com.yundrone.sunray/v1/sunray.pb.h"

namespace com::yundrone::sunray::v1 {

constexpr uint32_t kMinDirectControlLeaseMs = 250;
constexpr uint32_t kMaxDirectControlLeaseMs = 2000;
constexpr int kMaxWaypointCount = 256;

bool validate_uav_direct_control_goal(const UavDirectControlGoal& goal,
                                      std::string* error = nullptr);
bool validate_emergency_kill_goal(const EmergencyKillGoal& goal,
                                  std::string* error = nullptr);
bool validate_uav_waypoint_mission_goal(const UavWaypointMissionGoal& goal,
                                        std::string* error = nullptr);

}  // namespace com::yundrone::sunray::v1

#endif
