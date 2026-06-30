#include "ui/main_window.hpp"

#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

#include "common/monitor_ui_style.hpp"

QWidget* MainWindow::build_commands_page(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto* top_splitter = new QSplitter(Qt::Horizontal, page);
    auto* left_panel = new QWidget(top_splitter);
    auto* left_layout = new QVBoxLayout(left_panel);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->setSpacing(12);
    left_layout->addWidget(build_status_panel(left_panel));
    left_layout->addWidget(build_recent_issues_panel(left_panel));

    auto* actions_frame = new QFrame(left_panel);
    auto* actions_layout = new QHBoxLayout(actions_frame);
    actions_layout->setContentsMargins(0, 0, 0, 0);
    reconnect_button_ = new QPushButton("Reconnect now", actions_frame);
    monitor_ui::style_button(reconnect_button_, monitor_ui::ButtonRole::kSecondary);
    actions_layout->addWidget(reconnect_button_);
    actions_layout->addStretch(1);
    left_layout->addWidget(actions_frame);
    left_layout->addStretch(1);

    auto* right_panel = build_command_panel(top_splitter);
    top_splitter->addWidget(left_panel);
    top_splitter->addWidget(right_panel);
    top_splitter->setStretchFactor(0, 2);
    top_splitter->setStretchFactor(1, 3);
    top_splitter->setSizes({520, 760});

    auto* history_panel = build_command_history_panel(page);
    layout->addWidget(top_splitter, 0);
    layout->addWidget(history_panel, 1);

    connect(reconnect_button_, &QPushButton::clicked, this, [this]() {
        if (backend_ != nullptr) {
            backend_->request_reconnect_now();
        }
    });
    return page;
}

QWidget* MainWindow::build_status_panel(QWidget* parent) {
    auto* group = new QGroupBox("Command gate", parent);
    auto* grid = new QGridLayout(group);
    grid->setContentsMargins(12, 12, 12, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    command_status_value_ = new QLabel(group);
    command_peer_value_ = new QLabel(group);
    command_session_id_value_ = new QLabel(group);
    command_remote_value_ = new QLabel(group);
    command_tcp_listen_value_ = new QLabel(group);
    command_authority_value_ = new QLabel(group);
    command_note_value_ = new QLabel(group);
    command_error_value_ = new QLabel(group);
    command_note_value_->setWordWrap(true);
    command_error_value_->setWordWrap(true);
    command_remote_value_->setWordWrap(true);
    command_tcp_listen_value_->setWordWrap(true);
    monitor_ui::set_mono(command_peer_value_);
    monitor_ui::set_mono(command_session_id_value_);
    monitor_ui::set_mono(command_remote_value_);
    monitor_ui::set_mono(command_tcp_listen_value_);

    auto* status_label = new QLabel("链路 / session", group);
    auto* peer_label = new QLabel("peer id", group);
    auto* session_label = new QLabel("session id", group);
    auto* remote_label = new QLabel("对端", group);
    auto* listen_label = new QLabel("本地监听", group);
    auto* authority_label = new QLabel("Authority", group);
    auto* note_label = new QLabel("最近说明", group);
    auto* error_label = new QLabel("最近错误", group);

    grid->addWidget(status_label, 0, 0);
    grid->addWidget(command_status_value_, 0, 1);
    grid->addWidget(peer_label, 0, 2);
    grid->addWidget(command_peer_value_, 0, 3);
    grid->addWidget(session_label, 0, 4);
    grid->addWidget(command_session_id_value_, 0, 5);

    grid->addWidget(remote_label, 1, 0);
    grid->addWidget(command_remote_value_, 1, 1, 1, 3);
    grid->addWidget(listen_label, 1, 4);
    grid->addWidget(command_tcp_listen_value_, 1, 5);

    grid->addWidget(authority_label, 2, 0);
    grid->addWidget(command_authority_value_, 2, 1);
    grid->addWidget(note_label, 2, 2);
    grid->addWidget(command_note_value_, 2, 3, 1, 3);
    grid->addWidget(error_label, 3, 0);
    grid->addWidget(command_error_value_, 3, 1, 1, 5);

    grid->setColumnStretch(1, 3);
    grid->setColumnStretch(3, 2);
    grid->setColumnStretch(5, 3);
    return group;
}

QWidget* MainWindow::build_command_panel(QWidget* parent) {
    auto* group = new QGroupBox("Command workspace", parent);
    auto* layout = new QGridLayout(group);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(8);

    takeoff_height_spin_ = make_spin(0.6, 0.1, 20.0, 0.1);
    takeoff_velocity_spin_ = make_spin(0.8, 0.1, 5.0, 0.1);
    land_velocity_spin_ = make_spin(0.5, 0.1, 5.0, 0.1);
    return_loiter_spin_ = make_spin(0.0, 0.0, 30.0, 0.5);
    point_x_spin_ = make_spin(0.0, -100.0, 100.0, 0.1);
    point_y_spin_ = make_spin(0.0, -100.0, 100.0, 0.1);
    point_z_spin_ = make_spin(0.8, -10.0, 100.0, 0.1);
    point_yaw_spin_ = make_spin(0.0, -180.0, 180.0, 1.0, 1);
    vel_x_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_y_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_z_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_yaw_rate_spin_ = make_spin(0.0, -180.0, 180.0, 1.0, 1);

    takeoff_button_ = new QPushButton("Stage TAKEOFF", group);
    land_button_ = new QPushButton("Stage LAND", group);
    return_button_ = new QPushButton("Stage RETURN", group);
    point_button_ = new QPushButton("Send MOVE_POINT", group);
    velocity_button_ = new QPushButton("Send MOVE_VELOCITY", group);
    monitor_ui::style_button(takeoff_button_, monitor_ui::ButtonRole::kWarning);
    monitor_ui::style_button(land_button_, monitor_ui::ButtonRole::kWarning);
    monitor_ui::style_button(return_button_, monitor_ui::ButtonRole::kWarning);
    monitor_ui::style_button(point_button_, monitor_ui::ButtonRole::kPrimary);
    monitor_ui::style_button(velocity_button_, monitor_ui::ButtonRole::kPrimary);
    command_hint_label_ = new QLabel(group);
    command_hint_label_->setWordWrap(true);
    command_hint_label_->setStyleSheet("color:#56656d;");
    current_command_value_ = new QLabel("-", group);
    current_execution_state_value_ = new QLabel("-", group);
    current_battery_value_ = new QLabel("-", group);
    current_execution_reason_value_ = new QLabel("-", group);
    current_execution_reason_value_->setWordWrap(true);
    current_ready_takeoff_value_ = new QLabel("-", group);
    current_ready_land_value_ = new QLabel("-", group);

    connect(takeoff_button_, &QPushButton::clicked, this, &MainWindow::stage_takeoff);
    connect(land_button_, &QPushButton::clicked, this, &MainWindow::stage_land);
    connect(return_button_, &QPushButton::clicked, this, &MainWindow::stage_return);
    connect(point_button_, &QPushButton::clicked, this, &MainWindow::stage_move_point);
    connect(velocity_button_, &QPushButton::clicked, this, &MainWindow::stage_move_velocity);

    layout->addWidget(new QLabel("起飞高度(m)"), 0, 0);
    layout->addWidget(takeoff_height_spin_, 0, 1);
    layout->addWidget(new QLabel("起飞速度(m/s)"), 0, 2);
    layout->addWidget(takeoff_velocity_spin_, 0, 3);
    layout->addWidget(takeoff_button_, 0, 4);

    layout->addWidget(new QLabel("降落速度(m/s)"), 1, 0);
    layout->addWidget(land_velocity_spin_, 1, 1);
    layout->addWidget(new QLabel("返航盘旋(s)"), 1, 2);
    layout->addWidget(return_loiter_spin_, 1, 3);
    layout->addWidget(land_button_, 1, 4);
    layout->addWidget(return_button_, 1, 5);

    layout->addWidget(new QLabel("目标点 x/y/z/yaw(deg)"), 2, 0);
    layout->addWidget(point_x_spin_, 2, 1);
    layout->addWidget(point_y_spin_, 2, 2);
    layout->addWidget(point_z_spin_, 2, 3);
    layout->addWidget(point_yaw_spin_, 2, 4);
    layout->addWidget(point_button_, 2, 5);

    layout->addWidget(new QLabel("惯性系速度 vx/vy/vz/yaw_rate(deg/s)"), 3, 0);
    layout->addWidget(vel_x_spin_, 3, 1);
    layout->addWidget(vel_y_spin_, 3, 2);
    layout->addWidget(vel_z_spin_, 3, 3);
    layout->addWidget(vel_yaw_rate_spin_, 3, 4);
    layout->addWidget(velocity_button_, 3, 5);

    layout->addWidget(new QLabel("当前命令"), 4, 0);
    layout->addWidget(current_command_value_, 4, 1);
    layout->addWidget(new QLabel("阶段"), 4, 2);
    layout->addWidget(current_execution_state_value_, 4, 3);
    layout->addWidget(new QLabel("电量"), 4, 4);
    layout->addWidget(current_battery_value_, 4, 5);

    layout->addWidget(new QLabel("原因"), 5, 0);
    layout->addWidget(current_execution_reason_value_, 5, 1, 1, 3);
    layout->addWidget(new QLabel("可起飞"), 5, 4);
    layout->addWidget(current_ready_takeoff_value_, 5, 5);

    layout->addWidget(new QLabel("可降落"), 6, 0);
    layout->addWidget(current_ready_land_value_, 6, 1);
    layout->addWidget(command_hint_label_, 6, 2, 1, 4);
    return group;
}

QWidget* MainWindow::build_command_history_panel(QWidget* parent) {
    auto* group = new QGroupBox("Command history / ACK audit", parent);
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(8);

    command_history_table_ = new QTableWidget(group);
    command_history_table_->setColumnCount(7);
    command_history_table_->setHorizontalHeaderLabels(
        {"时间", "命令", "状态", "Session", "Message ID", "执行状态", "回执"});
    command_history_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    command_history_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    command_history_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    command_history_table_->verticalHeader()->setVisible(false);
    command_history_table_->horizontalHeader()->setStretchLastSection(false);
    command_history_table_->horizontalHeader()->setSectionResizeMode(0,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(1,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(2,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(3,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(4,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(5,
                                                                     QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    command_history_table_->setWordWrap(false);
    monitor_ui::style_table(command_history_table_);
    layout->addWidget(command_history_table_);
    return group;
}

QWidget* MainWindow::build_topics_panel(QWidget* parent) {
    auto* group = new QGroupBox("State telemetry", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* tabs = new QTabWidget(group);
    tabs->setDocumentMode(true);
    tabs->setStyleSheet("QTabBar::tab { padding:6px 12px; }");

    auto topics = backend_ != nullptr ? backend_->snapshot_topics() : make_default_monitor_topics();
    for (const auto& key : monitor_topic_display_order()) {
        const auto it = topics.find(key);
        if (it == topics.end()) {
            continue;
        }

        auto* page = new QWidget(tabs);
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(8);

        auto* summary = new QLabel(page);
        summary->setText("状态: WAIT | last update: -- | session: -- | message: --");
        monitor_ui::set_mono(summary);
        topic_summary_labels_[key] = summary;
        layout->addWidget(summary);

        auto* table = create_topic_table(page);
        topic_tables_[key] = table;
        layout->addWidget(table, 1);
        tabs->addTab(page, QString::fromStdString(it->second.title));
    }

    root->addWidget(tabs, 1);
    return group;
}

QWidget* MainWindow::build_log_panel(QWidget* parent) {
    return build_log_page_body(parent);
}
