#ifndef YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_MAP_HPP
#define YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_MAP_HPP

#include <string>
#include <unordered_map>

#include <yunlink/yunlink.hpp>

void fill_local_odom_from_yunlink(const yunlink::LocalOdomSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values);
void fill_odom_state_from_yunlink(const yunlink::OdomStateSnapshot& msg,
                                  std::unordered_map<std::string, std::string>& values);
void fill_control_cmd_from_yunlink(const yunlink::UavControlCmdSnapshot& msg,
                                   std::unordered_map<std::string, std::string>& values);
void fill_control_state_from_yunlink(const yunlink::UavControlStateSnapshot& msg,
                                     std::unordered_map<std::string, std::string>& values);
void fill_px4_state_from_yunlink(const yunlink::Px4StateSnapshot& msg,
                                 std::unordered_map<std::string, std::string>& values);

#endif  // YUNLINK_ADVANCED_MONITOR_MAPPING_VALUE_MAP_HPP
