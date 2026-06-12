#include "common/sunray_status_format.hpp"

#include <string>

namespace {

std::string unknown_with_value(const char* prefix, uint8_t value) {
    return std::string(prefix) + "=" + std::to_string(value);
}

}  // namespace

std::string uav_control_fsm_name(uint8_t value) {
    switch (value) {
    case 0:
        return "OFF";
    case 1:
        return "INIT";
    case 2:
        return "TAKEOFF";
    case 3:
        return "HOVER";
    case 4:
        return "RETURN";
    case 5:
        return "LAND";
    case 6:
        return "MOVE";
    case 7:
        return "KILL";
    default:
        return unknown_with_value("FSM", value);
    }
}

std::string uav_control_cmd_name(uint8_t value) {
    switch (value) {
    case 0:
        return "UNDEFINE";
    case 1:
        return "TAKEOFF";
    case 2:
        return "LAND";
    case 3:
        return "RETURN";
    case 4:
        return "KILL";
    case 5:
        return "HOVER";
    case 6:
        return "MOVE_POINT";
    case 7:
        return "MOVE_VELOCITY";
    case 8:
        return "MOVE_TRAJECTORY";
    case 9:
        return "MOVE_POINT_BODY";
    case 10:
        return "MOVE_VELOCITY_BODY";
    case 11:
        return "MOVE_POINT_WGS84";
    default:
        return unknown_with_value("CMD", value);
    }
}

std::string uav_control_cmd_source_name(uint8_t value) {
    switch (value) {
    case 0:
        return "UNDEFINE";
    case 1:
        return "SUNRAY_STATION";
    case 2:
        return "RC_CONTROLLER";
    case 3:
        return "TERMINAL";
    case 4:
        return "SWARM_CONTROL";
    case 5:
        return "PLANNING";
    case 6:
        return "EXAMPLE_DEMO";
    default:
        return unknown_with_value("SOURCE", value);
    }
}

std::string uav_yaw_mode_name(uint8_t value) {
    switch (value) {
    case 0:
        return "KEEP_YAW";
    case 1:
        return "SET_YAW";
    case 2:
        return "SET_YAWRATE";
    default:
        return unknown_with_value("YAW_MODE", value);
    }
}

std::string uav_controller_type_name(uint8_t value) {
    switch (value) {
    case 0:
        return "PX4_ORIGIN";
    case 1:
        return "GEOMETRIC";
    default:
        return unknown_with_value("CONTROLLER", value);
    }
}

std::string uav_controller_output_type_name(uint8_t value) {
    switch (value) {
    case 0:
        return "OUTPUT_NONE";
    case 1:
        return "POSITION_TARGET";
    case 2:
        return "ATTITUDE_TARGET";
    default:
        return unknown_with_value("OUTPUT", value);
    }
}

std::string localization_source_name(uint8_t value) {
    switch (value) {
    case 0:
        return "VIOBOT";
    case 1:
        return "MOCAP";
    case 2:
        return "VINS";
    case 3:
        return "GAZEBO";
    case 4:
        return "GAZEBO_ARUCO";
    case 5:
        return "PENGYU_SIM";
    case 6:
        return "FASTLIO_EKF";
    default:
        return unknown_with_value("LOCALIZATION", value);
    }
}

std::string px4_landed_state_name(uint8_t value) {
    switch (value) {
    case 0:
        return "UNDEFINED";
    case 1:
        return "ON_GROUND";
    case 2:
        return "IN_AIR";
    case 3:
        return "TAKEOFF";
    case 4:
        return "LANDING";
    default:
        return unknown_with_value("LANDED", value);
    }
}

std::string land_type_name(uint8_t value) {
    switch (value) {
    case 0:
        return "CTRL_LAND";
    case 1:
        return "PX4_AUTOLAND";
    default:
        return unknown_with_value("LAND_TYPE", value);
    }
}

std::string position_target_frame_name(uint8_t value) {
    switch (value) {
    case 1:
        return "LOCAL_NED";
    case 7:
        return "LOCAL_OFFSET_NED";
    case 8:
        return "BODY_NED";
    case 9:
        return "BODY_OFFSET_NED";
    default:
        return unknown_with_value("FRAME", value);
    }
}

std::string command_execution_state_name(uint8_t value) {
    switch (value) {
    case 0:
        return "IDLE";
    case 1:
        return "ACCEPTED";
    case 2:
        return "RUNNING";
    case 3:
        return "WAITING_PHYSICAL_STATE";
    case 4:
        return "SUCCEEDED";
    case 5:
        return "FAILED";
    case 6:
        return "CANCELLED";
    case 7:
        return "TIMEOUT";
    default:
        return unknown_with_value("EXEC", value);
    }
}

std::string command_kind_name(uint16_t value) {
    switch (value) {
    case 0:
        return "UNKNOWN";
    case 1:
        return "TAKEOFF";
    case 2:
        return "LAND";
    case 3:
        return "RETURN";
    case 4:
        return "MOVE_POINT";
    case 5:
        return "MOVE_VELOCITY";
    case 6:
        return "TRAJECTORY_CHUNK";
    case 7:
        return "FORMATION_TASK";
    default:
        return std::string("COMMAND_KIND=") + std::to_string(value);
    }
}
