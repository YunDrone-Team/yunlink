#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_COMMAND_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_COMMAND_MODEL_HPP

#include <string>

#include <yunlink/yunlink.hpp>

struct MonitorCommandDraft {
    bool valid{false};
    bool continuous{false};
    std::string summary;
    std::string detail;
};

std::string monitor_cmd_name_takeoff();
std::string monitor_cmd_name_land();
std::string monitor_cmd_name_return();
std::string monitor_cmd_name_goto();
std::string monitor_cmd_name_velocity(bool body_frame);

MonitorCommandDraft make_takeoff_draft(const yunlink::TakeoffCommand& cmd);
MonitorCommandDraft make_land_draft(const yunlink::LandCommand& cmd);
MonitorCommandDraft make_return_draft(const yunlink::ReturnCommand& cmd);
MonitorCommandDraft make_goto_draft(const yunlink::GotoCommand& cmd);
MonitorCommandDraft make_velocity_draft(const yunlink::VelocitySetpointCommand& cmd);

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_COMMAND_MODEL_HPP
