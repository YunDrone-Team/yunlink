#include "ui/main_window.hpp"

#include <QDateTime>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextCursor>

void MainWindow::refresh_logs(const std::vector<LogEntry>& logs) {
    if (pause_logs_checkbox_ != nullptr && pause_logs_checkbox_->isChecked()) {
        if (!logs.empty() && logs.back().sequence > rendered_last_sequence_) {
            pending_new_log_count_ =
                static_cast<int>(logs.back().sequence -
                                 std::max(rendered_last_sequence_, logs.front().sequence - 1));
            jump_to_latest_button_->setText(QString("查看新日志 (%1)").arg(pending_new_log_count_));
            jump_to_latest_button_->setVisible(true);
            sync_log_status();
        }
        return;
    }

    if (logs.empty()) {
        reset_log_view();
        sync_log_status();
        return;
    }

    const bool at_bottom_before_update = is_log_view_at_bottom();
    const bool should_follow = follow_latest_checkbox_ != nullptr &&
                               follow_latest_checkbox_->isChecked() && at_bottom_before_update;
    const int previous_scroll_value = logs_->verticalScrollBar()->value();
    const uint64_t previous_last_sequence = rendered_last_sequence_;

    if (rendered_last_sequence_ != 0 && logs.front().sequence > rendered_last_sequence_ + 1) {
        log_overflow_since_render_ = true;
        rebuild_log_view_from_entries(logs);
        sync_log_status();
        return;
    }

    bool appended = false;
    for (const auto& entry : logs) {
        if (entry.sequence <= rendered_last_sequence_) {
            continue;
        }
        append_log_entry(entry);
        appended = true;
    }

    if (!appended && rendered_log_groups_.empty()) {
        rebuild_log_view_from_entries(logs);
    }

    if (!should_follow) {
        suppress_log_scroll_events_ = true;
        logs_->verticalScrollBar()->setValue(previous_scroll_value);
        suppress_log_scroll_events_ = false;
    }

    if (should_follow || previous_last_sequence == 0) {
        pending_new_log_count_ = 0;
        jump_to_latest_button_->setVisible(false);
        scroll_logs_to_bottom();
    } else if (appended) {
        pending_new_log_count_ += static_cast<int>(logs.back().sequence - previous_last_sequence);
        jump_to_latest_button_->setText(QString("查看新日志 (%1)").arg(pending_new_log_count_));
        jump_to_latest_button_->setVisible(true);
    }
    sync_log_status();
}

void MainWindow::append_log_entry(const LogEntry& entry) {
    if (!rendered_log_groups_.empty()) {
        auto& last_group = rendered_log_groups_.back();
        if (last_group.entry.level == entry.level && last_group.entry.source == entry.source &&
            last_group.entry.message == entry.message) {
            last_group.entry.timestamp_ms = entry.timestamp_ms;
            last_group.entry.sequence = entry.sequence;
            ++last_group.repeat_count;
            update_last_log_group_block();
            rendered_last_sequence_ = entry.sequence;
            return;
        }
    }

    RenderedLogGroup group;
    group.entry = entry;
    rendered_log_groups_.push_back(group);
    append_log_group_block(rendered_log_groups_.back());
    rendered_last_sequence_ = entry.sequence;
    log_overflow_since_render_ = false;
}

void MainWindow::append_log_group_block(const RenderedLogGroup& group) {
    logs_->appendPlainText(format_log_line(group));
}

void MainWindow::update_last_log_group_block() {
    if (rendered_log_groups_.empty()) {
        return;
    }
    auto* document = logs_->document();
    QTextBlock block = document->lastBlock();
    if (!block.isValid()) {
        rebuild_log_view_from_entries(backend_->snapshot_logs());
        return;
    }
    QTextCursor cursor(block);
    cursor.select(QTextCursor::BlockUnderCursor);
    cursor.removeSelectedText();
    cursor.insertText(format_log_line(rendered_log_groups_.back()));
}

void MainWindow::rebuild_log_view_from_entries(const std::vector<LogEntry>& logs) {
    const bool should_follow =
        follow_latest_checkbox_ != nullptr && follow_latest_checkbox_->isChecked();
    const bool keep_bottom = should_follow || is_log_view_at_bottom();
    reset_log_view();
    for (const auto& entry : logs) {
        append_log_entry(entry);
    }
    pending_new_log_count_ = 0;
    jump_to_latest_button_->setVisible(false);
    if (keep_bottom) {
        scroll_logs_to_bottom();
    }
}

void MainWindow::reset_log_view() {
    rendered_log_groups_.clear();
    rendered_last_sequence_ = 0;
    pending_new_log_count_ = 0;
    log_overflow_since_render_ = false;
    logs_->clear();
}

void MainWindow::sync_log_status() {
    int rendered_rows = 0;
    for (const auto& group : rendered_log_groups_) {
        rendered_rows += group.repeat_count;
    }

    QString status = QString("显示 %1 条").arg(rendered_rows);
    if (pause_logs_checkbox_ != nullptr && pause_logs_checkbox_->isChecked()) {
        status += " | 已暂停";
    } else if (follow_latest_checkbox_ != nullptr && follow_latest_checkbox_->isChecked()) {
        status += " | 跟随最新";
    } else {
        status += " | 手动浏览";
    }
    if (pending_new_log_count_ > 0) {
        status += QString(" | 待看 %1 条").arg(pending_new_log_count_);
    }
    if (log_overflow_since_render_) {
        status += " | 已重建";
    }
    log_status_label_->setText(status);
}

void MainWindow::scroll_logs_to_bottom() {
    suppress_log_scroll_events_ = true;
    auto* bar = logs_->verticalScrollBar();
    bar->setValue(bar->maximum());
    suppress_log_scroll_events_ = false;
}

bool MainWindow::is_log_view_at_bottom() const {
    if (logs_ == nullptr) {
        return true;
    }
    const auto* bar = logs_->verticalScrollBar();
    return bar->value() >= bar->maximum() - 2;
}

void MainWindow::copy_selected_logs() {
    if (logs_ == nullptr) {
        return;
    }
    if (logs_->textCursor().hasSelection()) {
        logs_->copy();
        return;
    }
    logs_->selectAll();
    logs_->copy();
    auto cursor = logs_->textCursor();
    cursor.clearSelection();
    logs_->setTextCursor(cursor);
}

void MainWindow::export_logs() {
    const QString default_name = QString("compare_ui_logs_%1.txt")
                                     .arg(QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss"));
    const QString path = QFileDialog::getSaveFileName(
        this, "导出日志", default_name, "Text Files (*.txt);;All Files (*)");
    if (path.isEmpty()) {
        return;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "导出失败", "无法写入日志文件。");
        return;
    }
    std::vector<RenderedLogGroup> groups;
    for (const auto& entry : backend_->snapshot_logs()) {
        if (!groups.empty()) {
            auto& last = groups.back();
            if (last.entry.level == entry.level && last.entry.source == entry.source &&
                last.entry.message == entry.message) {
                last.entry.timestamp_ms = entry.timestamp_ms;
                last.entry.sequence = entry.sequence;
                ++last.repeat_count;
                continue;
            }
        }
        RenderedLogGroup group;
        group.entry = entry;
        groups.push_back(group);
    }
    QStringList lines;
    for (const auto& group : groups) {
        lines.push_back(format_log_line(group));
    }
    file.write(lines.join("\n").toUtf8());
    file.close();
}

QString MainWindow::format_log_line(const RenderedLogGroup& group) const {
    QString line = QString("[%1] [%2] [%3] %4")
                       .arg(format_log_timestamp(group.entry.timestamp_ms),
                            level_label(group.entry.level),
                            source_label(group.entry.source),
                            QString::fromStdString(group.entry.message));
    if (group.repeat_count > 1) {
        line += QString("  x%1").arg(group.repeat_count);
    }
    return line;
}

QString MainWindow::format_log_timestamp(const uint64_t timestamp_ms) {
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp_ms))
        .toString("HH:mm:ss.zzz");
}

QString MainWindow::level_label(const LogLevel level) {
    switch (level) {
    case LogLevel::kInfo:
        return "INFO";
    case LogLevel::kWarn:
        return "WARNING";
    case LogLevel::kError:
        return "ERROR";
    }
    return "INFO";
}

QString MainWindow::source_label(const LogSource source) {
    switch (source) {
    case LogSource::kCompare:
        return "Compare";
    case LogSource::kRos:
        return "ROS";
    case LogSource::kYunlink:
        return "Yunlink";
    case LogSource::kSession:
        return "Session";
    }
    return "Compare";
}
