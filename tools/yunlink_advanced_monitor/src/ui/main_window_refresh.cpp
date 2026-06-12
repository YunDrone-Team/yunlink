#include "ui/main_window.hpp"

#include <cctype>

#include <QDateTime>
#include <QScrollBar>

#include "common/sunray_status_format.hpp"

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

std::string topic_label_for_display(const std::string& label, const std::string& key) {
    std::string out = label;
    const bool is_yaw_rate =
        key.find("yaw_rate_radps") != std::string::npos ||
        key.find("body_rate_radps") != std::string::npos ||
        key.find("angular_radps") != std::string::npos;
    const bool is_yaw_angle =
        !is_yaw_rate &&
        (key.find("yaw_rad") != std::string::npos || key.find("desired_yaw_rad") != std::string::npos);
    if (is_yaw_rate) {
        for (std::string::size_type pos = 0;
             (pos = out.find("radps", pos)) != std::string::npos;
             pos += 5) {
            out.replace(pos, 5, "degps");
        }
        return out;
    }
    if (is_yaw_angle) {
        for (std::string::size_type pos = 0;
             (pos = out.find("rad", pos)) != std::string::npos;
             pos += 3) {
            out.replace(pos, 3, "deg");
        }
    }
    return out;
}

std::string topic_value_for_display(const MonitorTopicState& topic, const std::string& key) {
    const std::string value = topic_value_or_default(topic, key);
    if (value == "WAIT" || value == "--" || value == "<empty>") {
        return value;
    }

    double parsed = 0.0;
    const bool is_yaw_rate =
        key.find("yaw_rate_radps") != std::string::npos ||
        key.find("body_rate_radps") != std::string::npos ||
        key.find("angular_radps") != std::string::npos;
    const bool is_yaw_angle =
        !is_yaw_rate &&
        (key.find("yaw_rad") != std::string::npos || key.find("desired_yaw_rad") != std::string::npos);

    if ((is_yaw_angle || is_yaw_rate) && monitor_parse_double(value, &parsed)) {
        return is_yaw_rate ? monitor_fmt_degrees_per_sec(parsed) : monitor_fmt_degrees(parsed);
    }
    return value;
}

QString rich_status_text(const DeveloperStatusLine& line) {
    return QString("<span style=\"font-weight:600;color:%1;\">[%2]</span> %3<br><span style=\"color:#475467;\">%4</span>")
        .arg(QString::fromStdString(developer_status_level_color(line.level)))
        .arg(QString::fromStdString(developer_status_level_label(line.level)))
        .arg(QString::fromStdString(line.title).toHtmlEscaped())
        .arg(QString::fromStdString(line.detail).toHtmlEscaped());
}

QString card_rows_text(const DeveloperStatusCard& card) {
    QStringList lines;
    for (const auto& row : card.rows) {
        lines.append(QString("<b>%1</b>: %2")
                         .arg(QString::fromStdString(row.first).toHtmlEscaped())
                         .arg(QString::fromStdString(row.second).toHtmlEscaped()));
    }
    return lines.join("<br>");
}

}  // namespace

void MainWindow::refresh_view() {
    refresh_dashboard();
    refresh_status();
    refresh_discovery_devices(false);
    refresh_recent_issues();
    refresh_command_controls();
    refresh_command_history();
    refresh_system_services();
    refresh_topics();
    refresh_logs();
}

void MainWindow::refresh_dashboard() {
    if (backend_ == nullptr) {
        return;
    }

    const auto connection = backend_->snapshot_connection();
    const auto topics = backend_->snapshot_topics();
    const auto history = backend_->snapshot_command_history();
    const auto snapshot =
        build_developer_status_snapshot(connection,
                                        topics,
                                        history,
                                        static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()));

    auto set_card = [](QLabel* summary, QLabel* body, const DeveloperStatusCard& card) {
        if (summary != nullptr) {
            summary->setTextFormat(Qt::RichText);
            summary->setText(rich_status_text(card.summary));
        }
        if (body != nullptr) {
            body->setTextFormat(Qt::RichText);
            body->setText(card_rows_text(card));
        }
    };

    set_card(dashboard_yunlink_summary_, dashboard_yunlink_body_, snapshot.yunlink);
    set_card(dashboard_px4_summary_, dashboard_px4_body_, snapshot.px4);
    set_card(dashboard_localization_summary_, dashboard_localization_body_, snapshot.localization);
    set_card(dashboard_control_summary_, dashboard_control_body_, snapshot.control);
    set_card(dashboard_command_summary_, dashboard_command_body_, snapshot.command);

    if (dashboard_localization_panel_ != nullptr) {
        dashboard_localization_panel_->setTextFormat(Qt::RichText);
        dashboard_localization_panel_->setText(card_rows_text(snapshot.localization));
    }
    if (dashboard_control_panel_ != nullptr) {
        dashboard_control_panel_->setTextFormat(Qt::RichText);
        dashboard_control_panel_->setText(card_rows_text(snapshot.control));
    }
    if (dashboard_issues_value_ != nullptr) {
        QStringList lines;
        if (snapshot.issues.empty()) {
            lines.append("[OK] 无关键问题");
        } else {
            for (const auto& issue : snapshot.issues) {
                lines.append(QString("[%1] %2: %3")
                                 .arg(QString::fromStdString(developer_status_level_label(issue.level)))
                                 .arg(QString::fromStdString(issue.title).toHtmlEscaped())
                                 .arg(QString::fromStdString(issue.detail).toHtmlEscaped()));
            }
        }
        dashboard_issues_value_->setTextFormat(Qt::RichText);
        dashboard_issues_value_->setText(lines.join("<br>"));
    }
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

void MainWindow::refresh_recent_issues() {
    if (backend_ == nullptr || recent_issues_value_ == nullptr) {
        return;
    }

    const auto entries = backend_->snapshot_logs();
    QStringList lines;
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        if (it->level == MonitorLogLevel::kInfo) {
            continue;
        }
        lines.append(QString("[%1][%2] %3")
                         .arg(QString::fromStdString(source_label(it->source)))
                         .arg(QString::fromStdString(level_label(it->level)))
                         .arg(QString::fromStdString(it->message)));
        if (lines.size() >= 3) {
            break;
        }
    }
    if (lines.isEmpty()) {
        recent_issues_value_->setText("最近异常: 无");
        return;
    }
    recent_issues_value_->setText("最近异常:\n" + lines.join('\n'));
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
        set_item(table, row, 0, topic_label_for_display(field.label, field.key));
        set_item(table, row, 1, topic_value_for_display(topic, field.key));
    }
}

void MainWindow::refresh_logs() {
    if (backend_ == nullptr || logs_ == nullptr) {
        return;
    }

    const auto entries = backend_->snapshot_logs();
    const uint64_t last_sequence = entries.empty() ? 0 : entries.back().sequence;
    QStringList lines;
    lines.reserve(static_cast<int>(entries.size()));
    for (const auto& entry : entries) {
        if (!log_entry_visible(entry)) {
            continue;
        }
        lines.append(QString("[%1][%2][%3] %4")
                         .arg(format_timestamp(entry.timestamp_ms))
                         .arg(QString::fromStdString(level_label(entry.level)))
                         .arg(QString::fromStdString(source_label(entry.source)))
                         .arg(QString::fromStdString(entry.message)));
    }
    const int visible_count = lines.size();
    if (last_sequence == rendered_last_sequence_ && entries.size() == rendered_log_count_ &&
        visible_count == rendered_visible_log_count_) {
        return;
    }

    const bool follow = log_should_autofollow();
    logs_->setPlainText(lines.join('\n'));
    if (follow && logs_->verticalScrollBar() != nullptr) {
        logs_->verticalScrollBar()->setValue(logs_->verticalScrollBar()->maximum());
    }
    rendered_last_sequence_ = last_sequence;
    rendered_log_count_ = entries.size();
    rendered_visible_log_count_ = visible_count;
}
