#include "ui/main_window.hpp"

#include <cctype>
#include <QBrush>
#include <QColor>
#include <QScrollBar>

#include "model/monitor_compare.hpp"

namespace {

bool is_blank_topic_value(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    for (unsigned char ch : value) {
        if (!std::isspace(ch)) {
            return false;
        }
    }
    return true;
}

std::string normalize_topic_value(const std::string& value) {
    return is_blank_topic_value(value) ? std::string("<empty>") : value;
}

}  // namespace

void MainWindow::refresh_view() {
    refresh_status();
    refresh_command_controls();
    refresh_command_history();
    refresh_topics();
    refresh_logs();
}

void MainWindow::refresh_status() {
    if (backend_ == nullptr) {
        return;
    }

    const auto snapshot = backend_->snapshot_connection();
    const std::string compact_status = "runtime=" + snapshot.runtime_status +
                                       " session=" + snapshot.session_state +
                                       " link=" + snapshot.link_state;
    status_value_->setText(QString::fromStdString(compact_status));
    peer_value_->setText(snapshot.peer_id.empty() ? "-" : QString::fromStdString(snapshot.peer_id));
    session_id_value_->setText(
        snapshot.session_id == 0 ? "-" : QString::number(static_cast<qulonglong>(snapshot.session_id)));
    remote_value_->setText(QString::fromStdString(snapshot.remote_endpoint));
    tcp_listen_value_->setText(QString::fromStdString(snapshot.listen_endpoint));
    note_value_->setText(snapshot.last_note.empty() ? "-" : QString::fromStdString(snapshot.last_note));
    error_value_->setText(snapshot.last_error.empty() ? "-" : QString::fromStdString(snapshot.last_error));
}

void MainWindow::refresh_command_controls() {
    if (backend_ == nullptr) {
        return;
    }

    const bool ready = backend_->can_send_commands();
    if (takeoff_button_ != nullptr) {
        takeoff_button_->setEnabled(true);
        land_button_->setEnabled(true);
        return_button_->setEnabled(true);
        point_button_->setEnabled(true);
        body_point_button_->setEnabled(true);
        velocity_button_->setEnabled(true);
        body_velocity_button_->setEnabled(true);
    }

    command_hint_label_->setText(
        ready
            ? "当前 session 和 authority 已就绪。支持的按钮会真实下发 YunLink command；命令历史会显示 SENT / RECEIVED / APPLIED / TIMEOUT。"
            : "当前尚未拿到可发送状态。按钮仍可点击：已接入的命令会尝试申请 authority 并发送；未接入的命令会明确写日志提示。");
}

void MainWindow::refresh_command_history() {
    if (backend_ == nullptr || command_history_table_ == nullptr) {
        return;
    }

    const auto entries = backend_->snapshot_command_history();
    command_history_table_->setRowCount(static_cast<int>(entries.size()));
    for (int row = 0; row < static_cast<int>(entries.size()); ++row) {
        const auto& entry = entries[entries.size() - 1 - static_cast<size_t>(row)];
        set_item(command_history_table_, row, 0, format_timestamp(entry.sent_at_ms).toStdString());
        set_item(command_history_table_,
                 row,
                 1,
                 entry.action + (entry.detail.empty() ? std::string() : "\n" + entry.detail));
        set_item(command_history_table_, row, 2, command_lifecycle_label(entry.lifecycle));
        set_item(command_history_table_,
                 row,
                 3,
                 entry.session_id == 0 ? std::string("--") : monitor_fmt_num(entry.session_id));
        set_item(command_history_table_,
                 row,
                 4,
                 entry.message_id == 0 ? std::string("--") : monitor_fmt_num(entry.message_id));

        std::string result = entry.result_phase.empty() ? "--" : entry.result_phase;
        if (!entry.result_detail.empty()) {
            result += "\n" + entry.result_detail;
        }
        set_item(command_history_table_, row, 5, result);

        std::string applied = entry.applied_detail.empty() ? "--" : entry.applied_detail;
        set_item(command_history_table_, row, 6, applied);
    }
}

void MainWindow::refresh_topics() {
    if (backend_ == nullptr) {
        return;
    }

    const auto topics = backend_->snapshot_topics();
    for (const auto& key : monitor_topic_display_order()) {
        const auto it = topics.find(key);
        if (it == topics.end()) {
            continue;
        }
        refresh_topic(key, it->second);
    }
}

void MainWindow::refresh_topic(const std::string& key, const MonitorTopicState& topic) {
    auto table_it = topic_tables_.find(key);
    if (table_it == topic_tables_.end()) {
        return;
    }

    const double timeout_sec = backend_->state_timeout_sec();
    const bool ros_has_data = monitor_has_snapshot(topic.ros_latest);
    const bool yn_has_data = monitor_has_snapshot(topic.latest);
    const bool ros_fresh = ros_has_data && !topic.ros_latest.receive_time.isZero() &&
                           (ros::Time::now() - topic.ros_latest.receive_time).toSec() <= timeout_sec;
    const bool yn_fresh = yn_has_data && !topic.latest.receive_time.isZero() &&
                          (ros::Time::now() - topic.latest.receive_time).toSec() <= timeout_sec;

    auto* table = table_it->second;
    table->setRowCount(static_cast<int>(topic.rows.size()));
    for (int row = 0; row < static_cast<int>(topic.rows.size()); ++row) {
        const auto& field = topic.rows[static_cast<size_t>(row)];
        const auto ros_it = topic.ros_latest.values.find(field.key);
        const auto yn_it = topic.latest.values.find(field.key);
        const std::string ros_value = ros_it != topic.ros_latest.values.end()
                                          ? ros_it->second
                                          : (ros_has_data ? std::string("--") : std::string("WAIT"));
        const std::string yn_value = yn_it != topic.latest.values.end()
                                         ? yn_it->second
                                         : (yn_has_data ? std::string("--") : std::string("WAIT"));
        const std::string ros_display = normalize_topic_value(ros_value);
        const std::string yn_display = normalize_topic_value(yn_value);
        MonitorCompareResult outcome;
        if (!ros_has_data || !yn_has_data || ros_it == topic.ros_latest.values.end() ||
            yn_it == topic.latest.values.end()) {
            outcome = {};
        } else if (!ros_fresh || !yn_fresh) {
            outcome = monitor_stale_result(topic.ros_latest, topic.latest, timeout_sec);
        } else {
            outcome =
                monitor_compare_result(
                    key, field.key, topic.ros_latest, topic.latest, topic.source_dt_ms, topic.aligned_delay_ms);
        }

        set_item(table, row, 0, field.label);
        auto* ros_item = set_item(table, row, 1, ros_display);
        auto* yn_item = set_item(table, row, 2, yn_display);
        auto* result_item = set_item(table, row, 3, outcome.text);

        QColor bg(240, 243, 245);
        QColor fg(98, 107, 115);
        if (outcome.level == MonitorCompareLevel::kNormal) {
            bg = QColor(226, 245, 234);
            fg = QColor(26, 95, 56);
        } else if (outcome.level == MonitorCompareLevel::kAttention) {
            bg = QColor(255, 244, 208);
            fg = QColor(128, 94, 24);
        } else if (outcome.level == MonitorCompareLevel::kValueError ||
                   outcome.level == MonitorCompareLevel::kTimingError ||
                   outcome.level == MonitorCompareLevel::kStale ||
                   outcome.level == MonitorCompareLevel::kLargeDelay) {
            bg = QColor(251, 228, 228);
            fg = QColor(142, 32, 32);
        }
        const QBrush bg_brush(bg);
        const QBrush fg_brush(fg);
        ros_item->setBackground(bg_brush);
        yn_item->setBackground(bg_brush);
        result_item->setBackground(bg_brush);
        result_item->setForeground(fg_brush);
    }
}

void MainWindow::refresh_logs() {
    if (backend_ == nullptr) {
        return;
    }

    const auto entries = backend_->snapshot_logs();
    const uint64_t last_sequence = entries.empty() ? 0 : entries.back().sequence;
    if (last_sequence == rendered_last_sequence_ && entries.size() == rendered_log_count_) {
        return;
    }

    QStringList lines;
    lines.reserve(static_cast<int>(entries.size()));
    for (const auto& entry : entries) {
        lines.append(QString("[%1][%2][%3] %4")
                         .arg(format_timestamp(entry.timestamp_ms))
                         .arg(QString::fromStdString(level_label(entry.level)))
                         .arg(QString::fromStdString(source_label(entry.source)))
                         .arg(QString::fromStdString(entry.message)));
    }
    logs_->setPlainText(lines.join('\n'));
    logs_->verticalScrollBar()->setValue(logs_->verticalScrollBar()->maximum());
    rendered_last_sequence_ = last_sequence;
    rendered_log_count_ = entries.size();
}
