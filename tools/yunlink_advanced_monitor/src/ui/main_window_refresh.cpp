#include "ui/main_window.hpp"

#include <cctype>

#include <QScrollBar>

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

std::string topic_value_or_default(const MonitorTopicState& topic, const std::string& key) {
    const auto it = topic.latest.values.find(key);
    if (it != topic.latest.values.end()) {
        return normalize_topic_value(it->second);
    }
    return monitor_has_snapshot(topic.latest) ? std::string("--") : std::string("WAIT");
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
    authority_value_->setText(snapshot.authority_state.empty()
                                  ? "-"
                                  : QString::fromStdString(snapshot.authority_state));
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
        velocity_button_->setEnabled(true);
    }

    command_hint_label_->setText(
        ready ? "当前 session 与 authority 已就绪，按钮会真实下发 YunLink command。"
              : "当前尚未拿到可发送状态，monitor 会继续等待会话与 authority。");
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

        std::string result = entry.result_phase.empty() ? std::string("--") : entry.result_phase;
        if (!entry.result_detail.empty()) {
            result += "\n" + entry.result_detail;
        }
        set_item(command_history_table_, row, 5, result);
        set_item(command_history_table_,
                 row,
                 6,
                 entry.applied_detail.empty() ? std::string("--") : entry.applied_detail);
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

    auto* table = table_it->second;
    table->setRowCount(static_cast<int>(topic.rows.size()));
    for (int row = 0; row < static_cast<int>(topic.rows.size()); ++row) {
        const auto& field = topic.rows[static_cast<size_t>(row)];
        set_item(table, row, 0, field.label);
        set_item(table, row, 1, topic_value_or_default(topic, field.key));
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
