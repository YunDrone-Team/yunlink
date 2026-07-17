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

void MainWindow::stage_refresh_runtime_logs() {
    if (backend_ != nullptr) {
        backend_->request_runtime_log_list();
    }
}

void MainWindow::stage_read_selected_runtime_log() {
    if (backend_ == nullptr || runtime_log_list_widget_ == nullptr) {
        return;
    }
    const auto* item = runtime_log_list_widget_->currentItem();
    if (item == nullptr) {
        return;
    }
    const std::string runtime_id = item->data(Qt::UserRole).toString().toStdString();
    const auto state = backend_->snapshot_system_services();
    for (const auto& runtime : state.runtime_logs) {
        if (runtime.runtime_id == runtime_id) {
            backend_->request_runtime_log_read(runtime_id, runtime.cursor);
            return;
        }
    }
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

    auto* runtime_row = new QHBoxLayout();
    refresh_runtime_logs_button_ = new QPushButton("刷新运行日志", group);
    read_runtime_log_button_ = new QPushButton("读取选中日志", group);
    monitor_ui::style_button(refresh_runtime_logs_button_, monitor_ui::ButtonRole::kSecondary);
    monitor_ui::style_button(read_runtime_log_button_, monitor_ui::ButtonRole::kSecondary);
    runtime_row->addWidget(new QLabel("运行日志", group));
    runtime_row->addWidget(refresh_runtime_logs_button_);
    runtime_row->addWidget(read_runtime_log_button_);
    runtime_row->addStretch(1);
    root->addLayout(runtime_row);

    auto* runtime_splitter = new QSplitter(Qt::Horizontal, group);
    runtime_log_list_widget_ = new QListWidget(runtime_splitter);
    runtime_log_list_widget_->setSelectionMode(QAbstractItemView::SingleSelection);
    runtime_log_text_ = new QPlainTextEdit(runtime_splitter);
    monitor_ui::configure_copyable_log_view(runtime_log_text_);
    runtime_splitter->setStretchFactor(0, 1);
    runtime_splitter->setStretchFactor(1, 3);
    root->addWidget(runtime_splitter, 1);

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
        {"时间", "请求", "目标", "状态", "会话", "结果"});
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
    connect(refresh_runtime_logs_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_refresh_runtime_logs);
    connect(read_runtime_log_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_read_selected_runtime_log);
    connect(runtime_log_list_widget_,
            &QListWidget::itemClicked,
            this,
            [this](QListWidgetItem*) { stage_read_selected_runtime_log(); });
    return group;
}
