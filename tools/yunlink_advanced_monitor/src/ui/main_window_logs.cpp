#include "ui/main_window.hpp"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QStringList>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"

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
    auto* group = new QGroupBox("Audit log", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* actions = new QHBoxLayout();
    log_filter_combo_ = new QComboBox(group);
    log_filter_combo_->addItems({"全部日志", "仅 Warn / Error", "Connection", "Authority", "Command", "System"});
    log_autofollow_checkbox_ = new QCheckBox("自动跟随", group);
    log_autofollow_checkbox_->setChecked(log_autofollow_);
    clear_logs_button_ = new QPushButton("Clear log", group);
    monitor_ui::style_button(clear_logs_button_, monitor_ui::ButtonRole::kSecondary);
    actions->addWidget(log_filter_combo_);
    actions->addWidget(log_autofollow_checkbox_);
    actions->addWidget(clear_logs_button_);
    actions->addStretch(1);
    root->addLayout(actions);

    logs_ = new QPlainTextEdit(group);
    logs_->setReadOnly(true);
    logs_->setLineWrapMode(QPlainTextEdit::NoWrap);
    monitor_ui::style_log_view(logs_);
    root->addWidget(logs_, 1);

    connect(log_filter_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
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
    case 1:
        return entry.level != MonitorLogLevel::kInfo;
    case 2:
        return entry.source == MonitorLogSource::kConnection;
    case 3:
        return entry.source == MonitorLogSource::kAuthority;
    case 4:
        return entry.source == MonitorLogSource::kCommand;
    case 5:
        return entry.source == MonitorLogSource::kSystemService;
    default:
        return true;
    }
}

bool MainWindow::log_should_autofollow() const {
    if (logs_ == nullptr) {
        return false;
    }
    if (log_autofollow_) {
        return true;
    }
    auto* bar = logs_->verticalScrollBar();
    return bar != nullptr && bar->value() >= bar->maximum() - 4;
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
        lines.append(QString("%1  %2  %3  %4")
                         .arg(format_timestamp(entry.timestamp_ms), -13)
                         .arg(QString::fromStdString(level_label(entry.level)), -5)
                         .arg(QString::fromStdString(source_label(entry.source)), -10)
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
