#include "model/monitor_topics.hpp"

namespace {

MonitorFieldDef field(std::string key, std::string label) {
    MonitorFieldDef view;
    view.key = std::move(key);
    view.label = std::move(label);
    return view;
}

}  // namespace

MonitorTopicState make_command_execution_status_topic() {
    MonitorTopicState topic;
    topic.key = "command_execution_status";
    topic.title = "command_execution_status";
    topic.yunlink_name = "CommandExecutionStatusSnapshot";
    topic.rows = {
        field("header.frame_id", "Header frame | header.frame_id"),
        field("header.stamp_ns", "Header stamp_ns | header.stamp_ns"),
        field("agent_name", "机器人名称 | agent_name"),
        field("agent_id", "机器人编号 | agent_id"),
        field("session_id", "YunLink session_id | session_id"),
        field("command_message_id", "命令 message_id | command_message_id"),
        field("command_correlation_id", "命令 correlation_id | command_correlation_id"),
        field("command_kind", "命令类型 | command_kind"),
        field("command_kind_name", "命令类型名称 | command_kind_name"),
        field("execution_state", "执行状态 | execution_state"),
        field("execution_state_name", "执行状态名称 | execution_state_name"),
        field("active", "当前占用控制器 | active"),
        field("terminal", "是否终态 | terminal"),
        field("success", "是否成功 | success"),
        field("result_code", "结果码 | result_code"),
        field("detail", "执行细节 | detail"),
        field("control_state", "控制 FSM 状态 | control_state"),
        field("control_state_name", "控制 FSM 状态名 | control_state_name"),
        field("px4_landed_state", "PX4 landed_state | px4_landed_state"),
        field("px4_landed_state_name", "PX4 landed_state 名称 | px4_landed_state_name"),
        field("ready_for_takeoff", "可起飞 | ready_for_takeoff"),
        field("ready_for_land", "可降落 | ready_for_land"),
        field("busy_reason", "忙碌原因 | busy_reason"),
    };
    return topic;
}
