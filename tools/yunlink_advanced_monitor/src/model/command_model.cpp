#include "model/command_model.hpp"

#include <sstream>

#include "common/monitor_format.hpp"

namespace {

std::string format_float(double value) {
    return monitor_fmt_float(value);
}

std::string format_deg(double value_rad) {
    return monitor_fmt_degrees(value_rad);
}

std::string format_degps(double value_radps) {
    return monitor_fmt_degrees_per_sec(value_radps);
}

}  // namespace

std::string monitor_cmd_name_takeoff() {
    return "TAKEOFF";
}

std::string monitor_cmd_name_land() {
    return "LAND";
}

std::string monitor_cmd_name_return() {
    return "RETURN";
}

std::string monitor_cmd_name_goto() {
    return "MOVE_POINT";
}

std::string monitor_cmd_name_velocity(bool body_frame) {
    return body_frame ? "MOVE_VELOCITY_BODY" : "MOVE_VELOCITY";
}

MonitorCommandDraft make_takeoff_draft(const yunlink::TakeoffCommand& cmd) {
    MonitorCommandDraft draft;
    draft.valid = true;
    draft.summary = monitor_cmd_name_takeoff();
    std::ostringstream oss;
    oss << "relative_height_m=" << format_float(cmd.relative_height_m)
        << " max_velocity_mps=" << format_float(cmd.max_velocity_mps);
    draft.detail = oss.str();
    return draft;
}

MonitorCommandDraft make_land_draft(const yunlink::LandCommand& cmd) {
    MonitorCommandDraft draft;
    draft.valid = true;
    draft.summary = monitor_cmd_name_land();
    draft.detail = "max_velocity_mps=" + format_float(cmd.max_velocity_mps);
    return draft;
}

MonitorCommandDraft make_return_draft(const yunlink::ReturnCommand& cmd) {
    MonitorCommandDraft draft;
    draft.valid = true;
    draft.summary = monitor_cmd_name_return();
    draft.detail = "loiter_before_return_s=" + format_float(cmd.loiter_before_return_s);
    return draft;
}

MonitorCommandDraft make_goto_draft(const yunlink::GotoCommand& cmd) {
    MonitorCommandDraft draft;
    draft.valid = true;
    draft.summary = monitor_cmd_name_goto();
    std::ostringstream oss;
    oss << "x_m=" << format_float(cmd.x_m) << " y_m=" << format_float(cmd.y_m)
        << " z_m=" << format_float(cmd.z_m) << " yaw_deg=" << format_deg(cmd.yaw_rad);
    draft.detail = oss.str();
    return draft;
}

MonitorCommandDraft make_velocity_draft(const yunlink::VelocitySetpointCommand& cmd) {
    MonitorCommandDraft draft;
    draft.valid = true;
    draft.continuous = true;
    draft.summary = monitor_cmd_name_velocity(cmd.body_frame);
    std::ostringstream oss;
    if (cmd.body_frame) {
        oss << "body_vx_mps=" << format_float(cmd.vx_mps)
            << " body_vy_mps=" << format_float(cmd.vy_mps)
            << " yaw_rate_degps=" << format_degps(cmd.yaw_rate_radps)
            << " fixed_height_m=bridge-current-px4-state";
    } else {
        oss << "vx_mps=" << format_float(cmd.vx_mps) << " vy_mps=" << format_float(cmd.vy_mps)
            << " vz_mps=" << format_float(cmd.vz_mps)
            << " yaw_rate_degps=" << format_degps(cmd.yaw_rate_radps)
            << " body_frame=false";
    }
    draft.detail = oss.str();
    return draft;
}
