#include "ui/main_window.hpp"

#include <QListWidget>
#include <QListWidgetItem>
#include <QStringList>

#include "common/monitor_ui_style.hpp"
#include "ui/system_service/system_service_ui_helpers.hpp"

using monitor_system_service_ui::bool_label;
using monitor_system_service_ui::join_values;
using monitor_system_service_ui::update_feature_list_if_changed;
using monitor_system_service_ui::update_plain_text_if_changed;

void MainWindow::refresh_system_services() {
    if (backend_ == nullptr || feature_list_widget_ == nullptr || feature_detail_text_ == nullptr ||
        system_service_history_table_ == nullptr) {
        return;
    }

    const auto state = backend_->snapshot_system_services();
    const auto history = backend_->snapshot_system_service_history();

    const QString selected_feature =
        feature_name_edit_ == nullptr ? "" : feature_name_edit_->text().trimmed();
    update_feature_list_if_changed(
        feature_list_widget_, state.last_status, state.feature_names, selected_feature);

    if (feature_request_preview_ != nullptr) {
        const QString feature = feature_name_edit_ == nullptr ? "" : feature_name_edit_->text().trimmed();
        const QString args =
            feature_override_args_edit_ == nullptr ? "" : feature_override_args_edit_->text().trimmed();
        feature_request_preview_->setText(monitor_ui::feature_request_preview(
            feature,
            args,
            feature_restart_checkbox_ != nullptr && feature_restart_checkbox_->isChecked(),
            feature_terminal_checkbox_ != nullptr && feature_terminal_checkbox_->isChecked(),
            feature_force_stop_checkbox_ != nullptr && feature_force_stop_checkbox_->isChecked()));
    }

    const std::string selected_name =
        feature_name_edit_ == nullptr ? std::string() : feature_name_edit_->text().trimmed().toStdString();
    QStringList detail_lines;
    if (!selected_name.empty()) {
        const auto it = state.feature_details.find(selected_name);
        if (it == state.feature_details.end()) {
            detail_lines.append("WAIT");
        } else {
            const auto& detail = it->second;
            detail_lines.append(QString::fromStdString("success: " + bool_label(detail.success)));
            detail_lines.append(QString::fromStdString("message: " + detail.message));
            detail_lines.append(QString::fromStdString("name: " + detail.name));
            detail_lines.append(QString::fromStdString("group: " + detail.group));
            detail_lines.append(QString::fromStdString("running: " + bool_label(detail.running)));
            detail_lines.append(QString::fromStdString("auto_start: " + bool_label(detail.auto_start)));
            detail_lines.append(QString::fromStdString("description: " + detail.description));
            detail_lines.append(QString::fromStdString(
                "last_action: " + (detail.last_action.empty() ? std::string("--") : detail.last_action)));
            detail_lines.append(QString::fromStdString(
                "last_action_message: " +
                (detail.last_action_message.empty() ? std::string("--") : detail.last_action_message)));
            detail_lines.append(QString::fromStdString(
                "depends_on: " + (detail.depends_on.empty() ? std::string("--")
                                                            : join_values(detail.depends_on, ", "))));
            detail_lines.append(QString::fromStdString(
                "start_preview_units: " +
                (detail.start_preview_units.empty() ? std::string("--")
                                                    : join_values(detail.start_preview_units, ", "))));
            detail_lines.append(QString::fromStdString(
                "start_preview_commands: " +
                (detail.start_preview_commands.empty()
                     ? std::string("--")
                     : join_values(detail.start_preview_commands, "\n"))));
        }
    }
    update_plain_text_if_changed(feature_detail_text_, detail_lines.join('\n'));

    if (runtime_log_list_widget_ != nullptr) {
        const QString selected_runtime = runtime_log_list_widget_->currentItem() == nullptr
                                             ? QString()
                                             : runtime_log_list_widget_->currentItem()
                                                   ->data(Qt::UserRole)
                                                   .toString();
        runtime_log_list_widget_->clear();
        for (const auto& runtime : state.runtime_logs) {
            const QString label = QString::fromStdString(runtime.title.empty() ? runtime.runtime_id
                                                                                : runtime.title) +
                                  " [" + QString::fromStdString(runtime.state) + "]";
            auto* item = new QListWidgetItem(label, runtime_log_list_widget_);
            item->setData(Qt::UserRole, QString::fromStdString(runtime.runtime_id));
            if (item->data(Qt::UserRole).toString() == selected_runtime) {
                runtime_log_list_widget_->setCurrentItem(item);
            }
        }
    }
    if (runtime_log_text_ != nullptr && runtime_log_list_widget_ != nullptr &&
        runtime_log_list_widget_->currentItem() != nullptr) {
        const std::string selected =
            runtime_log_list_widget_->currentItem()->data(Qt::UserRole).toString().toStdString();
        for (const auto& runtime : state.runtime_logs) {
            if (runtime.runtime_id == selected) {
                update_plain_text_if_changed(runtime_log_text_, QString::fromStdString(runtime.chunk));
                break;
            }
        }
    }

    system_service_history_table_->setRowCount(static_cast<int>(history.size()));
    for (int row = 0; row < static_cast<int>(history.size()); ++row) {
        const auto& entry = history[history.size() - 1 - static_cast<size_t>(row)];
        set_item(system_service_history_table_, row, 0, format_timestamp(entry.sent_at_ms).toStdString());
        set_item(system_service_history_table_, row, 1, entry.action);
        set_item(system_service_history_table_, row, 2, entry.feature_name.empty() ? "--" : entry.feature_name);
        auto* status_item = set_item(system_service_history_table_,
                                     row,
                                     3,
                                     system_service_lifecycle_label(entry.lifecycle));
        monitor_ui::set_status_item(
            status_item, monitor_ui::level_from_status(system_service_lifecycle_label(entry.lifecycle)));
        set_item(system_service_history_table_,
                 row,
                 4,
                 entry.session_id == 0 ? std::string("--") : monitor_fmt_num(entry.session_id));
        set_item(system_service_history_table_,
                 row,
                 5,
                 entry.result_message.empty() ? std::string("--") : entry.result_message);
    }
}
