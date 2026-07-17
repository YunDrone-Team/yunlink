#include "ui/main_window.hpp"

#include <QDateTime>
#include <QScrollBar>

#include "common/monitor_ui_style.hpp"

namespace {

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
    if (page_stack_ == nullptr) {
        return;
    }
    if (backend_ != nullptr && page_nav_buttons_.size() > 4) {
        const bool config_supported = backend_->snapshot_configuration().supported;
        page_nav_buttons_[4]->setVisible(config_supported);
        if (!config_supported && page_stack_->currentIndex() == 4) {
            set_current_page(0);
            return;
        }
    }
    switch (page_stack_->currentIndex()) {
    case 0:
        refresh_status();
        refresh_dashboard();
        break;
    case 1:
        refresh_discovery_devices(false);
        break;
    case 2:
        refresh_status();
        refresh_recent_issues();
        refresh_command_controls();
        refresh_command_history();
        break;
    case 3:
        refresh_system_services();
        break;
    case 4:
        refresh_configuration();
        break;
    case 5:
        refresh_topics();
        break;
    case 6:
        refresh_packets();
        refresh_packet_flow();
        break;
    case 7:
        refresh_logs();
        break;
    default:
        break;
    }
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
        if (snapshot.issues.empty()) {
            dashboard_issues_value_->setTextFormat(Qt::RichText);
            dashboard_issues_value_->setText(monitor_ui::inline_notice_html(
                monitor_ui::Level::kOk, "无阻塞问题", "当前没有关键 WARN / ERROR。"));
        } else {
            QStringList lines;
            for (const auto& issue : snapshot.issues) {
                lines.append(QString("[%1] %2: %3")
                                 .arg(QString::fromStdString(developer_status_level_label(issue.level)))
                                 .arg(QString::fromStdString(issue.title).toHtmlEscaped())
                                 .arg(QString::fromStdString(issue.detail).toHtmlEscaped()));
            }
            dashboard_issues_value_->setTextFormat(Qt::RichText);
            dashboard_issues_value_->setText(monitor_ui::inline_notice_html(
                monitor_ui::Level::kWarn, "阻塞问题", lines.join("\n")));
        }
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
    const auto status_level = monitor_ui::level_from_status(compact_status);
    const auto authority_level = monitor_ui::level_from_status(snapshot.authority_state);
    const QString peer = snapshot.peer_id.empty() ? "-" : QString::fromStdString(snapshot.peer_id);
    const QString session =
        snapshot.session_id == 0 ? "-" : QString::number(static_cast<qulonglong>(snapshot.session_id));
    const QString remote = QString::fromStdString(snapshot.remote_endpoint);
    const QString listen = QString::fromStdString(snapshot.listen_endpoint);
    const QString authority =
        snapshot.authority_state.empty() ? "-" : QString::fromStdString(snapshot.authority_state);
    const QString note = snapshot.last_note.empty() ? "-" : QString::fromStdString(snapshot.last_note);
    const QString error = snapshot.last_error.empty() ? "-" : QString::fromStdString(snapshot.last_error);

    auto set_text = [](QLabel* label, const QString& text) {
        if (label != nullptr) {
            label->setText(text);
        }
    };
    auto set_tag = [](QLabel* label, monitor_ui::Level level, const QString& text) {
        if (label != nullptr) {
            monitor_ui::set_tag(label, level, text);
        }
    };

    set_tag(status_value_, status_level, QString::fromStdString(compact_status));
    set_text(peer_value_, peer);
    set_text(session_id_value_, session);
    set_text(remote_value_, remote);
    set_text(tcp_listen_value_, listen);
    set_tag(authority_value_, authority_level, authority);
    set_text(note_value_, note);
    set_text(error_value_, error);

    set_tag(command_status_value_, status_level, QString::fromStdString(compact_status));
    set_text(command_peer_value_, peer);
    set_text(command_session_id_value_, session);
    set_text(command_remote_value_, remote);
    set_text(command_tcp_listen_value_, listen);
    set_tag(command_authority_value_, authority_level, authority);
    set_text(command_note_value_, note);
    set_text(command_error_value_, error);
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
        recent_issues_value_->setText(monitor_ui::inline_notice_html(
            monitor_ui::Level::kOk, "最近异常", "无。当前日志没有 WARN / ERROR。"));
        return;
    }
    recent_issues_value_->setText(monitor_ui::inline_notice_html(
        monitor_ui::Level::kWarn, "最近异常", lines.join("\n")));
}
