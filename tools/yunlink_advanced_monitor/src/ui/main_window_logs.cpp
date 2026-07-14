#include "ui/main_window.hpp"

#include <algorithm>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStringList>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"

namespace {

bool is_command_status_log(const MonitorLogEntry& entry) {
    return entry.source == MonitorLogSource::kCommand &&
           entry.message.find("command_execution_status ") == 0;
}

bool is_command_event_log(const MonitorLogEntry& entry) {
    return entry.source == MonitorLogSource::kCommand && !is_command_status_log(entry);
}

}  // namespace

QWidget* MainWindow::build_recent_issues_panel(QWidget* parent) {
    auto* group = new QGroupBox("最近异常", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(0);
    recent_issues_value_ = new QLabel(group);
    recent_issues_value_->setWordWrap(true);
    recent_issues_value_->setTextFormat(Qt::RichText);
    root->addWidget(recent_issues_value_);
    return group;
}

QWidget* MainWindow::build_log_page_body(QWidget* parent) {
    auto* group = new QGroupBox("审计日志", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* actions = new QHBoxLayout();
    log_filter_combo_ = new QComboBox(group);
    log_filter_combo_->addItems({"全部日志(不含状态)",
                                 "全部日志",
                                 "仅 WARN / ERROR",
                                 "连接",
                                 "控制权",
                                 "命令事件",
                                 "命令状态",
                                 "桥接",
                                 "系统服务",
                                 "调试"});
    log_autofollow_checkbox_ = new QCheckBox("自动跟随", group);
    log_autofollow_checkbox_->setChecked(log_autofollow_);
    clear_logs_button_ = new QPushButton("清空日志", group);
    monitor_ui::style_button(clear_logs_button_, monitor_ui::ButtonRole::kSecondary);
    actions->addWidget(log_filter_combo_);
    actions->addWidget(log_autofollow_checkbox_);
    actions->addWidget(clear_logs_button_);
    actions->addStretch(1);
    root->addLayout(actions);

    logs_ = new QPlainTextEdit(group);
    monitor_ui::configure_copyable_log_view(logs_);
    root->addWidget(logs_, 1);

    connect(log_filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
        if (backend_ != nullptr) {
            backend_->set_debug_stream_enabled(log_filter_combo_ != nullptr &&
                                               log_filter_combo_->currentIndex() == 9);
        }
        rendered_last_sequence_ = 0;
        rendered_visible_log_count_ = -1;
        refresh_logs();
        refresh_recent_issues();
    });
    connect(log_autofollow_checkbox_, &QCheckBox::toggled, this, &MainWindow::stage_toggle_log_autofollow);
    connect(clear_logs_button_, &QPushButton::clicked, this, [this]() {
        if (backend_ != nullptr) {
            backend_->clear_logs();
        }
        rendered_last_sequence_ = 0;
        rendered_log_count_ = 0;
        rendered_visible_log_count_ = -1;
        if (logs_ != nullptr) {
            logs_->clear();
        }
        refresh_recent_issues();
    });
    return group;
}

bool MainWindow::log_entry_visible(const MonitorLogEntry& entry) const {
    if (log_filter_combo_ == nullptr) {
        return true;
    }
    switch (log_filter_combo_->currentIndex()) {
    case 0:
        return entry.source != MonitorLogSource::kDebug && !is_command_status_log(entry);
    case 1:
        return entry.source != MonitorLogSource::kDebug;
    case 2:
        return entry.level != MonitorLogLevel::kInfo;
    case 3:
        return entry.source == MonitorLogSource::kConnection;
    case 4:
        return entry.source == MonitorLogSource::kAuthority;
    case 5:
        return is_command_event_log(entry);
    case 6:
        return is_command_status_log(entry);
    case 7:
        return entry.source == MonitorLogSource::kBridge;
    case 8:
        return entry.source == MonitorLogSource::kSystemService;
    case 9:
        return entry.source == MonitorLogSource::kDebug;
    default:
        return true;
    }
}

bool MainWindow::log_should_autofollow() const {
    return logs_ != nullptr && log_autofollow_;
}

void MainWindow::stage_toggle_log_autofollow(bool checked) {
    log_autofollow_ = checked;
    if (checked && logs_ != nullptr && logs_->verticalScrollBar() != nullptr) {
        logs_->verticalScrollBar()->setValue(logs_->verticalScrollBar()->maximum());
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
        QString message = QString::fromStdString(entry.message);
        if (entry.repeat_count > 0) {
            const uint64_t first = entry.repeat_first_ms == 0 ? entry.timestamp_ms : entry.repeat_first_ms;
            const uint64_t last = entry.repeat_last_ms == 0 ? entry.timestamp_ms : entry.repeat_last_ms;
            message += QString("  [在 %2 秒内重复 %1 次]")
                           .arg(entry.repeat_count)
                           .arg((last > first ? last - first : 0) / 1000);
        }
        lines.append(QString("%1  %2  %3  %4")
                         .arg(format_timestamp(entry.timestamp_ms), -13)
                         .arg(QString::fromStdString(level_label(entry.level)), -5)
                         .arg(QString::fromStdString(source_label(entry.source)), -10)
                         .arg(message));
    }
    const int visible_count = lines.size();
    if (last_sequence == rendered_last_sequence_ && entries.size() == rendered_log_count_ &&
        visible_count == rendered_visible_log_count_) {
        return;
    }
    if (logs_->textCursor().hasSelection()) {
        return;
    }

    const bool follow = log_should_autofollow();
    auto* vertical_bar = logs_->verticalScrollBar();
    auto* horizontal_bar = logs_->horizontalScrollBar();
    const int previous_vertical_value = vertical_bar != nullptr ? vertical_bar->value() : 0;
    const int previous_horizontal_value = horizontal_bar != nullptr ? horizontal_bar->value() : 0;
    const bool horizontal_at_right =
        horizontal_bar != nullptr && previous_horizontal_value == horizontal_bar->maximum();
    logs_->setPlainText(lines.join('\n'));
    if (vertical_bar != nullptr) {
        if (follow) {
            vertical_bar->setValue(vertical_bar->maximum());
        } else {
            vertical_bar->setValue(std::min(previous_vertical_value, vertical_bar->maximum()));
        }
    }
    if (horizontal_bar != nullptr) {
        const int next_horizontal_value =
            horizontal_at_right ? horizontal_bar->maximum()
                                : std::min(previous_horizontal_value, horizontal_bar->maximum());
        horizontal_bar->setValue(next_horizontal_value);
    }
    rendered_last_sequence_ = last_sequence;
    rendered_log_count_ = entries.size();
    rendered_visible_log_count_ = visible_count;
}
