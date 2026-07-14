#include "dashboard/developer_status_model_internal.hpp"

namespace developer_status_detail {

DeveloperStatusCard build_yunlink_card(const MonitorConnectionSnapshot& connection,
                                       std::vector<DeveloperStatusLine>& issues) {
    DeveloperStatusLevel level = DeveloperStatusLevel::kOk;
    std::string detail = "runtime/session 正常";
    if (!connection.runtime_started || connection.session_id == 0) {
        level = DeveloperStatusLevel::kError;
        detail = "runtime 未启动或 session_id=0";
        add_issue(issues, level, "YunLink", detail);
    } else if (!connection.peer_ready) {
        level = DeveloperStatusLevel::kWarn;
        detail = "peer 尚未 ready";
        add_issue(issues, level, "YunLink", detail);
    }

    DeveloperStatusCard card;
    card.summary = make_line(level, "YunLink", detail);
    push_row(card.rows, "运行时", connection.runtime_status);
    push_row(card.rows,
             "Session",
             connection.session_state + " / " +
                 (connection.session_id == 0 ? std::string("-")
                                             : std::to_string(connection.session_id)));
    push_row(card.rows, "控制权", connection.authority_state.empty() ? "-" : connection.authority_state);
    push_row(card.rows, "Peer", connection.peer_id.empty() ? "-" : connection.peer_id);
    push_row(card.rows, "对端", connection.remote_endpoint);
    return card;
}

DeveloperStatusCard build_px4_card(const MonitorTopicState* px4_topic,
                                   uint64_t now_ms,
                                   std::vector<DeveloperStatusLine>& issues) {
    DeveloperStatusCard card;
    card.summary.title = "PX4";
    if (px4_topic == nullptr || !monitor_has_snapshot(px4_topic->latest)) {
        card.summary = make_line(DeveloperStatusLevel::kWarn, "PX4", "px4_state WAIT");
        add_issue(issues, card.summary.level, "PX4", card.summary.detail);
        push_row(card.rows, "快照", "WAIT");
        return card;
    }

    const auto& values = px4_topic->latest.values;
    DeveloperStatusLevel level = age_level(px4_topic, now_ms);
    std::string detail = "已连接";
    if (!topic_bool(values, "connected")) {
        level = DeveloperStatusLevel::kError;
        detail = "PX4 未连接";
        add_issue(issues, level, "PX4", detail);
    } else {
        const double battery = topic_double(values, "battery_percentage", 0.0);
        if (battery > 0.0 && battery < 0.2) {
            level = max_level(level, DeveloperStatusLevel::kWarn);
            detail = "电池低: " + monitor_fmt_percent(battery);
            add_issue(issues, DeveloperStatusLevel::kWarn, "PX4", detail);
        } else if (level != DeveloperStatusLevel::kOk) {
            detail = "消息 age=" + snapshot_age_text(px4_topic, now_ms);
        }
    }

    card.summary = make_line(level, "PX4", detail);
    push_row(card.rows,
             "链路",
             "connected=" + topic_value(values, "connected") + " armed=" + topic_value(values, "armed") +
                 " rc=" + topic_value(values, "rc_available"));
    push_row(card.rows,
             "飞行",
             "mode=" + topic_value(values, "flight_mode") + " status=" + topic_value(values, "system_status") +
                 " landed=" + topic_value(values, "landed_state_name"));
    push_row(card.rows,
             "电池",
             topic_value(values, "battery_voltage_v") + " V / " + topic_value(values, "battery_current_a") +
                 " A / " + monitor_fmt_percent(topic_double(values, "battery_percentage", 0.0)));
    push_row(card.rows, "本地位置", xyz_text(values, "local_pose.position_m"));
    push_row(card.rows,
             "设定点",
             "frame=" + topic_value(values, "setpoint_coordinate_frame") + " mask=" +
                 topic_value(values, "setpoint_local_type_mask"));
    push_row(card.rows, "消息时效", snapshot_age_text(px4_topic, now_ms));
    return card;
}

DeveloperStatusCard build_localization_card(const MonitorTopicState* odom_topic,
                                            const MonitorTopicState* diagnostic_topic,
                                            uint64_t now_ms,
                                            std::vector<DeveloperStatusLine>& issues) {
    DeveloperStatusCard card;
    card.summary.title = "定位";
    const bool has_diag = diagnostic_topic != nullptr && monitor_has_snapshot(diagnostic_topic->latest);
    if (has_diag) {
        const auto& diag = diagnostic_topic->latest.values;
        DeveloperStatusLevel level = DeveloperStatusLevel::kOk;
        std::string detail = "诊断正常";
        for (const auto* key : {"external_odom", "odom_state", "local_odom", "global_odom"}) {
            merge_topic_diagnostic_status(&level, &detail, key, topic_value(diag, std::string(key) + ".status", "OK"));
        }
        if (level != DeveloperStatusLevel::kOk) {
            add_issue(issues, level, "定位", detail);
        }
        card.summary = make_line(level, "定位", detail);
        push_row(card.rows,
                 "外部里程计",
                 topic_value(diag, "external_odom.status") + " / pub=" +
                     topic_value(diag, "external_odom.publisher_count") + " / hz=" +
                     topic_value(diag, "external_odom.hz") + " / age=" +
                     topic_value(diag, "external_odom.age_ms") + " ms");
        push_row(card.rows,
                 "odom_state",
                 topic_value(diag, "odom_state.status") + " / age=" +
                     topic_value(diag, "odom_state.age_ms") + " ms");
        push_row(card.rows,
                 "局部 / 全局",
                 "local=" + topic_value(diag, "local_odom.status") + " global=" +
                     topic_value(diag, "global_odom.status"));
        push_row(card.rows, "摘要", detail);
        return card;
    }

    if (odom_topic == nullptr || !monitor_has_snapshot(odom_topic->latest)) {
        card.summary = make_line(DeveloperStatusLevel::kWarn, "定位", "odom_state WAIT");
        add_issue(issues, card.summary.level, "定位", card.summary.detail);
        push_row(card.rows, "诊断", "WAIT");
        return card;
    }

    const auto& values = odom_topic->latest.values;
    DeveloperStatusLevel level = age_level(odom_topic, now_ms);
    std::string detail = "odometry_valid";
    if (!topic_bool(values, "odometry_valid")) {
        level = DeveloperStatusLevel::kError;
        detail = "odometry_valid=false";
        add_issue(issues, level, "定位", detail);
    } else if (topic_double(values, "odometry_update_hz", 0.0) <= 0.1) {
        level = max_level(level, DeveloperStatusLevel::kWarn);
        detail = "odometry_update_hz <= 0.1";
        add_issue(issues, DeveloperStatusLevel::kWarn, "定位", detail);
    } else if (level != DeveloperStatusLevel::kOk) {
        detail = "消息 age=" + snapshot_age_text(odom_topic, now_ms);
    }

    card.summary = make_line(level, "定位", detail);
    push_row(card.rows,
             "来源",
             topic_value(values, "external_source_name") + " / hz=" + topic_value(values, "odometry_update_hz"));
    push_row(card.rows, "外部 topic", topic_value(values, "subtopic_name_external_odom"));
    push_row(card.rows,
             "坐标系",
             topic_value(values, "global_frame_name") + " / " + topic_value(values, "local_frame_name") +
                 " / " + topic_value(values, "base_frame_name"));
    push_row(card.rows, "局部里程计", xyz_text(values, "local_odom.pose.position_m"));
    push_row(card.rows, "局部姿态", euler_text(values, "local_odom.pose."));
    push_row(card.rows, "全局里程计", xyz_text(values, "global_odom.pose.position_m"));
    push_row(card.rows, "全局姿态", euler_text(values, "global_odom.pose."));
    push_row(card.rows, "诊断", "WAIT");
    return card;
}

}  // namespace developer_status_detail
