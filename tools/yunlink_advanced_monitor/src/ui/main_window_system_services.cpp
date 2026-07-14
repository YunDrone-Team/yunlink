#include "ui/main_window.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QSplitter>
#include <QStringList>
#include <QVBoxLayout>
#include <QCheckBox>

#include "common/monitor_ui_style.hpp"
#include "ui/system_service/system_service_ui_helpers.hpp"

using monitor_system_service_ui::bool_label;
using monitor_system_service_ui::join_values;
using monitor_system_service_ui::split_override_args;
using monitor_system_service_ui::update_feature_list_if_changed;
using monitor_system_service_ui::update_plain_text_if_changed;

void MainWindow::stage_refresh_feature_list() {
    if (backend_ != nullptr) {
        backend_->request_feature_list();
    }
}

void MainWindow::stage_refresh_feature_detail() {
    if (backend_ == nullptr || feature_name_edit_ == nullptr) {
        return;
    }
    backend_->request_feature_get(feature_name_edit_->text().trimmed().toStdString());
}

void MainWindow::stage_start_feature() {
    if (backend_ == nullptr || feature_name_edit_ == nullptr) {
        return;
    }
    backend_->request_feature_start(feature_name_edit_->text().trimmed().toStdString(),
                                    split_override_args(feature_override_args_edit_ == nullptr
                                                            ? std::string()
                                                            : feature_override_args_edit_->text()
                                                                  .trimmed()
                                                                  .toStdString()),
                                    feature_restart_checkbox_ != nullptr &&
                                        feature_restart_checkbox_->isChecked(),
                                    feature_terminal_checkbox_ != nullptr &&
                                        feature_terminal_checkbox_->isChecked());
}

void MainWindow::stage_stop_feature() {
    if (backend_ == nullptr || feature_name_edit_ == nullptr) {
        return;
    }
    const bool force =
        feature_force_stop_checkbox_ != nullptr && feature_force_stop_checkbox_->isChecked();
    const QString feature_name = feature_name_edit_->text().trimmed();
    if (force &&
        !monitor_ui::confirm_warning(
            this,
            "确认强制 FeatureStop",
            "force_stop 会要求对端尽快停止目标 feature，可能中断正在运行的任务或日志输出。\n\nfeature=" +
                feature_name)) {
        return;
    }
    backend_->request_feature_stop(feature_name.toStdString(), force);
}

QWidget* MainWindow::build_system_service_panel(QWidget* parent) {
    auto* group = new QGroupBox("功能服务调试台", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* action_row = new QHBoxLayout();
    refresh_feature_list_button_ = new QPushButton("刷新功能列表 (FeatureList)", group);
    feature_name_edit_ = new QLineEdit(group);
    feature_name_edit_->setPlaceholderText("功能名称 (feature_name)");
    refresh_feature_detail_button_ = new QPushButton("查询功能详情 (FeatureGet)", group);
    monitor_ui::style_button(refresh_feature_list_button_, monitor_ui::ButtonRole::kSecondary);
    monitor_ui::style_button(refresh_feature_detail_button_, monitor_ui::ButtonRole::kSecondary);
    action_row->addWidget(refresh_feature_list_button_);
    action_row->addWidget(feature_name_edit_, 1);
    action_row->addWidget(refresh_feature_detail_button_);
    root->addLayout(action_row);

    auto* control_row = new QHBoxLayout();
    feature_override_args_edit_ = new QLineEdit(group);
    feature_override_args_edit_->setPlaceholderText("覆盖参数，逗号分隔 (override_args)");
    feature_restart_checkbox_ = new QCheckBox("运行中则重启", group);
    feature_restart_checkbox_->setToolTip("restart_if_running");
    feature_terminal_checkbox_ = new QCheckBox("使用终端启动", group);
    feature_terminal_checkbox_->setToolTip("start_with_terminal");
    feature_force_stop_checkbox_ = new QCheckBox("强制停止", group);
    feature_force_stop_checkbox_->setToolTip("force_stop");
    start_feature_button_ = new QPushButton("发送启动请求 (FeatureStart)", group);
    stop_feature_button_ = new QPushButton("发送停止请求 (FeatureStop)", group);
    monitor_ui::style_button(start_feature_button_, monitor_ui::ButtonRole::kPrimary);
    monitor_ui::style_button(stop_feature_button_, monitor_ui::ButtonRole::kWarning);
    control_row->addWidget(feature_override_args_edit_, 1);
    control_row->addWidget(feature_restart_checkbox_);
    control_row->addWidget(feature_terminal_checkbox_);
    control_row->addWidget(feature_force_stop_checkbox_);
    control_row->addWidget(start_feature_button_);
    control_row->addWidget(stop_feature_button_);
    root->addLayout(control_row);

    feature_list_widget_ = new QListWidget(group);
    feature_list_widget_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    feature_list_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
    feature_list_widget_->setStyleSheet("QListWidget { background:#ffffff; border:1px solid #c6c6c6;"
                                        " selection-background-color:#d0e2ff; selection-color:#161616; }");
    feature_detail_text_ = new QPlainTextEdit(group);
    monitor_ui::configure_copyable_log_view(feature_detail_text_);

    feature_request_preview_ = new QLabel(group);
    feature_request_preview_->setTextFormat(Qt::RichText);
    feature_request_preview_->setWordWrap(true);

    system_service_history_table_ = new QTableWidget(group);
    system_service_history_table_->setColumnCount(6);
    system_service_history_table_->setHorizontalHeaderLabels(
        {"时间", "请求", "功能", "状态", "会话", "结果"});
    system_service_history_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    system_service_history_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    system_service_history_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    system_service_history_table_->verticalHeader()->setVisible(false);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    system_service_history_table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    monitor_ui::style_table(system_service_history_table_);

    auto* content = new QSplitter(Qt::Horizontal, group);
    auto* left = new QWidget(content);
    auto* left_layout = new QVBoxLayout(left);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->addWidget(new QLabel("功能列表 (FeatureList)", left));
    left_layout->addWidget(feature_list_widget_, 1);
    left_layout->addWidget(new QLabel("请求历史", left));
    left_layout->addWidget(system_service_history_table_, 1);

    auto* right = new QWidget(content);
    auto* right_layout = new QVBoxLayout(right);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->addWidget(new QLabel("请求预览", right));
    right_layout->addWidget(feature_request_preview_);
    right_layout->addWidget(new QLabel("功能详情 (FeatureGet)", right));
    right_layout->addWidget(feature_detail_text_, 1);

    content->addWidget(left);
    content->addWidget(right);
    content->setStretchFactor(0, 3);
    content->setStretchFactor(1, 2);
    root->addWidget(content, 1);

    connect(refresh_feature_list_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_refresh_feature_list);
    connect(refresh_feature_detail_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_refresh_feature_detail);
    connect(feature_name_edit_, &QLineEdit::returnPressed, this, &MainWindow::stage_refresh_feature_detail);
    connect(feature_list_widget_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        if (feature_name_edit_ == nullptr || item == nullptr) {
            return;
        }
        const QString feature_name = item->data(Qt::UserRole).toString();
        if (!feature_name.isEmpty()) {
            feature_name_edit_->setText(feature_name);
        }
    });
    connect(start_feature_button_, &QPushButton::clicked, this, &MainWindow::stage_start_feature);
    connect(stop_feature_button_, &QPushButton::clicked, this, &MainWindow::stage_stop_feature);
    return group;
}

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
