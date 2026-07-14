#include "dashboard/developer_status_model_internal.hpp"

namespace developer_status_detail {

DeveloperStatusCard build_control_card(const MonitorTopicState* control_topic,
                                       uint64_t now_ms,
                                       std::vector<DeveloperStatusLine>& issues) {
    DeveloperStatusCard card;
    card.summary.title = "控制";
    if (control_topic == nullptr || !monitor_has_snapshot(control_topic->latest)) {
        card.summary = make_line(DeveloperStatusLevel::kWarn, "控制", "uav_control_state WAIT");
        add_issue(issues, card.summary.level, "控制", card.summary.detail);
        push_row(card.rows, "快照", "WAIT");
        return card;
    }

    const auto& values = control_topic->latest.values;
    DeveloperStatusLevel level = age_level(control_topic, now_ms);
    std::string detail = topic_value(values, "control_state_name");
    if (topic_bool(values, "odometry_lost") || !topic_bool(values, "odometry_valid")) {
        level = DeveloperStatusLevel::kError;
        detail = "odometry_lost 或 odometry_valid=false";
        add_issue(issues, level, "控制", detail);
    } else {
        const std::string fsm = topic_value(values, "control_state_name");
        if (fsm == "KILL" || fsm.rfind("FSM=", 0) == 0) {
            level = DeveloperStatusLevel::kError;
            detail = "FSM=" + fsm;
            add_issue(issues, level, "控制", detail);
        } else if (level != DeveloperStatusLevel::kOk) {
            detail = "消息 age=" + snapshot_age_text(control_topic, now_ms);
        }
    }

    card.summary = make_line(level, "控制", detail);
    push_row(card.rows,
             "Agent",
             topic_value(values, "agent_name") + topic_value(values, "agent_id") + " / controller=" +
                 topic_value(values, "controller_types_name"));
    push_row(card.rows,
             "FSM",
             topic_value(values, "control_state_name") + " / output=" +
                 topic_value(values, "controller_output_type_name"));
    push_row(card.rows,
             "里程计",
             "valid=" + topic_value(values, "odometry_valid") + " lost=" +
                 topic_value(values, "odometry_lost"));
    push_row(card.rows,
             "起飞 / 降落",
             "h=" + topic_value(values, "takeoff_relative_height_m") + " vmax=" +
                 topic_value(values, "takeoff_max_velocity_mps") + " land=" +
                 topic_value(values, "land_type_name") + " / " +
                 topic_value(values, "land_max_velocity_mps"));
    push_row(card.rows, "自身里程计", xyz_text(values, "self_odom.pose.position_m"));
    push_row(card.rows, "自身姿态", euler_text(values, "self_odom.pose."));
    push_row(card.rows, "消息时效", snapshot_age_text(control_topic, now_ms));
    return card;
}

DeveloperStatusCard build_command_card(const MonitorTopicState* cmd_topic,
                                       const MonitorTopicState* diagnostic_topic,
                                       const MonitorTopicState* control_topic,
                                       const std::vector<MonitorCommandHistoryEntry>& history,
                                       uint64_t now_ms,
                                       std::vector<DeveloperStatusLine>& issues) {
    DeveloperStatusCard card;
    card.summary.title = "命令";
    DeveloperStatusLevel level = DeveloperStatusLevel::kOk;
    std::string detail = "空闲";

    if (!history.empty()) {
        const auto& last = history.back();
        if (last.lifecycle == MonitorCommandLifecycle::kFailed ||
            last.lifecycle == MonitorCommandLifecycle::kTimeout) {
            level = DeveloperStatusLevel::kError;
            detail = last.action + " " + command_lifecycle_label(last.lifecycle);
            add_issue(issues, level, "命令", detail);
        } else {
            level = command_lifecycle_is_terminal(last.lifecycle) ? DeveloperStatusLevel::kOk
                                                                  : DeveloperStatusLevel::kWarn;
            detail = last.action + " " + command_lifecycle_label(last.lifecycle);
        }
        push_row(card.rows, "历史", detail);
    } else {
        push_row(card.rows, "历史", "无命令历史");
    }

    if (cmd_topic != nullptr && monitor_has_snapshot(cmd_topic->latest)) {
        const auto& values = cmd_topic->latest.values;
        push_row(card.rows,
                 "原始命令",
                 topic_value(values, "cmd_source_name") + " / " + topic_value(values, "control_cmd_name"));
        push_row(card.rows,
                 "原始输入",
                 xyz_text(values, "desired_pos_m") + " vel " + xyz_text(values, "desired_vel_mps"));
        push_row(card.rows, "原始命令时效", snapshot_age_text(cmd_topic, now_ms));
        if (cmd_topic->freshness_policy != MonitorTopicFreshnessPolicy::kSparseCommand) {
            level = max_level(level, age_level(cmd_topic, now_ms));
        }
    } else {
        push_row(card.rows, "原始命令", "WAIT");
    }

    if (diagnostic_topic != nullptr && monitor_has_snapshot(diagnostic_topic->latest)) {
        const auto& diag = diagnostic_topic->latest.values;
        const std::string cmd_status = topic_value(diag, "uav_control_cmd.status", "OK");
        push_row(card.rows,
                 "原始状态",
                 cmd_status + " / pub=" + topic_value(diag, "uav_control_cmd.publisher_count") + " / hz=" +
                     topic_value(diag, "uav_control_cmd.hz") + " / age=" +
                     topic_value(diag, "uav_control_cmd.age_ms") + " ms");
        const DeveloperStatusLevel cmd_status_level = topic_diagnostic_level(cmd_status);
        if (cmd_status_level != DeveloperStatusLevel::kOk) {
            if (should_replace_summary(level, detail, cmd_status_level)) {
                detail = "uav_control_cmd: " + cmd_status;
            }
            level = max_level(level, cmd_status_level);
            add_issue(issues, cmd_status_level, "命令", "uav_control_cmd: " + cmd_status);
        }
    } else {
        push_row(card.rows, "原始状态", "WAIT");
    }

    if (control_topic != nullptr && monitor_has_snapshot(control_topic->latest)) {
        const auto& values = control_topic->latest.values;
        push_row(card.rows,
                 "已接纳",
                 topic_value(values, "last_cmd.cmd_source_name") + " / " +
                     topic_value(values, "last_cmd.control_cmd_name"));
        push_row(card.rows,
                 "偏航模式",
                 topic_value(values, "last_cmd.yaw_mode_name") + " / stamp " +
                     topic_value(values, "last_cmd.header.stamp_ns"));
        push_row(card.rows,
                 "输出",
                 topic_value(values, "controller_output_type_name") + " / frame " +
                     topic_value(values, "position_target.coordinate_frame_name"));
    }

    card.summary = make_line(level, "命令", detail);
    return card;
}

}  // namespace developer_status_detail
