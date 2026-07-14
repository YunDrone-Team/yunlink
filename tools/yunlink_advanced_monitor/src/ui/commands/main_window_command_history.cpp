#include "ui/main_window.hpp"

#include "common/monitor_ui_style.hpp"
#include "common/sunray_status_format.hpp"

void MainWindow::refresh_command_history() {
    if (backend_ == nullptr || command_history_table_ == nullptr) {
        return;
    }

    const auto entries = backend_->snapshot_command_history();
    command_history_table_->setRowCount(static_cast<int>(entries.size()));
    for (int row = 0; row < static_cast<int>(entries.size()); ++row) {
        const auto& entry = entries[entries.size() - 1 - static_cast<size_t>(row)];
        set_item(command_history_table_, row, 0, format_timestamp(entry.sent_at_ms).toStdString());
        auto* command_item =
            set_item(command_history_table_,
                     row,
                     1,
                     entry.action + (entry.detail.empty() ? std::string() : "\n" + entry.detail));
        command_item->setToolTip(command_item->text());
        auto* lifecycle_item =
            set_item(command_history_table_, row, 2, command_lifecycle_label(entry.lifecycle));
        monitor_ui::set_status_item(
            lifecycle_item,
            monitor_ui::level_from_status(command_lifecycle_label(entry.lifecycle)));
        set_item(command_history_table_,
                 row,
                 3,
                 entry.session_id == 0 ? std::string("--") : monitor_fmt_num(entry.session_id));
        set_item(command_history_table_,
                 row,
                 4,
                 entry.message_id == 0 ? std::string("--") : monitor_fmt_num(entry.message_id));

        std::string execution =
            entry.execution_state.empty() ? std::string("--") : entry.execution_state;
        if (entry.has_execution_status) {
            execution += "\nready_takeoff=" + std::string(entry.ready_for_takeoff ? "yes" : "no") +
                         " ready_land=" + std::string(entry.ready_for_land ? "yes" : "no");
        }
        if (!entry.execution_detail.empty()) {
            execution += "\n" + entry.execution_detail;
        }
        auto* execution_item = set_item(command_history_table_, row, 5, execution);
        execution_item->setToolTip(execution_item->text());

        std::string result = entry.result_phase.empty() ? std::string("--") : entry.result_phase;
        if (!entry.result_detail.empty()) {
            result += "\n" + entry.result_detail;
        }
        auto* result_item = set_item(command_history_table_, row, 6, result);
        result_item->setToolTip(result_item->text());
    }
}
