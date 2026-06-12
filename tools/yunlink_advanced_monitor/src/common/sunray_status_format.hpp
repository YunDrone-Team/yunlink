#ifndef YUNLINK_ADVANCED_MONITOR_COMMON_SUNRAY_STATUS_FORMAT_HPP
#define YUNLINK_ADVANCED_MONITOR_COMMON_SUNRAY_STATUS_FORMAT_HPP

#include <cstdint>
#include <string>

std::string uav_control_fsm_name(uint8_t value);
std::string uav_control_cmd_name(uint8_t value);
std::string uav_control_cmd_source_name(uint8_t value);
std::string uav_yaw_mode_name(uint8_t value);
std::string uav_controller_type_name(uint8_t value);
std::string uav_controller_output_type_name(uint8_t value);
std::string localization_source_name(uint8_t value);
std::string px4_landed_state_name(uint8_t value);
std::string land_type_name(uint8_t value);
std::string position_target_frame_name(uint8_t value);
std::string command_execution_state_name(uint8_t value);
std::string command_kind_name(uint16_t value);

#endif  // YUNLINK_ADVANCED_MONITOR_COMMON_SUNRAY_STATUS_FORMAT_HPP
