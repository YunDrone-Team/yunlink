#include "mapping/value_map.hpp"

#include "mapping/value_setters.hpp"

void fill_mavros_state_from_ros(const mavros_msgs::State& msg,
                                std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_value(values, "guided", fmt_bool(msg.guided));
    set_value(values, "manual_input", fmt_bool(msg.manual_input));
    set_value(values, "mode", msg.mode);
    set_numeric(values, "system_status", msg.system_status);
}

void fill_mavros_state_from_yunlink(const yunlink::MavrosStateSnapshot& msg,
                                    std::unordered_map<std::string, std::string>& values) {
    set_header(values, "", msg.header);
    set_value(values, "connected", fmt_bool(msg.connected));
    set_value(values, "armed", fmt_bool(msg.armed));
    set_value(values, "guided", fmt_bool(msg.guided));
    set_value(values, "manual_input", fmt_bool(msg.manual_input));
    set_value(values, "mode", msg.mode);
    set_numeric(values, "system_status", msg.system_status);
}
