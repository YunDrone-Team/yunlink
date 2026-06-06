#include "ui/main_window.hpp"

#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QWidget>

void MainWindow::build_ui() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(12);

    auto* top_splitter = new QSplitter(Qt::Horizontal, central);
    top_splitter->addWidget(build_status_panel(top_splitter));
    top_splitter->addWidget(build_command_panel(top_splitter));
    top_splitter->setStretchFactor(0, 2);
    top_splitter->setStretchFactor(1, 3);
    top_splitter->setSizes({440, 860});

    auto* main_splitter = new QSplitter(Qt::Vertical, central);
    main_splitter->addWidget(top_splitter);
    main_splitter->addWidget(build_command_history_panel(main_splitter));
    main_splitter->addWidget(build_topics_panel(main_splitter));
    main_splitter->addWidget(build_log_panel(main_splitter));
    main_splitter->setStretchFactor(0, 0);
    main_splitter->setStretchFactor(1, 1);
    main_splitter->setStretchFactor(2, 4);
    main_splitter->setStretchFactor(3, 2);
    main_splitter->setSizes({220, 180, 520, 220});

    layout->addWidget(main_splitter, 1);

    setCentralWidget(central);
    refresh_view();
}

QWidget* MainWindow::build_status_panel(QWidget* parent) {
    auto* group = new QGroupBox("会话状态", parent);
    auto* grid = new QGridLayout(group);
    grid->setContentsMargins(12, 12, 12, 12);
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(8);

    status_value_ = new QLabel(group);
    peer_value_ = new QLabel(group);
    session_id_value_ = new QLabel(group);
    remote_value_ = new QLabel(group);
    tcp_listen_value_ = new QLabel(group);
    authority_value_ = new QLabel(group);
    note_value_ = new QLabel(group);
    error_value_ = new QLabel(group);
    note_value_->setWordWrap(true);
    error_value_->setWordWrap(true);
    remote_value_->setWordWrap(true);
    tcp_listen_value_->setWordWrap(true);

    auto* status_label = new QLabel("状态", group);
    auto* peer_label = new QLabel("Peer ID", group);
    auto* session_label = new QLabel("Session ID", group);
    auto* remote_label = new QLabel("对端", group);
    auto* listen_label = new QLabel("本地监听", group);
    auto* authority_label = new QLabel("Authority", group);
    auto* note_label = new QLabel("最近说明", group);
    auto* error_label = new QLabel("最近错误", group);

    grid->addWidget(status_label, 0, 0);
    grid->addWidget(status_value_, 0, 1);
    grid->addWidget(peer_label, 0, 2);
    grid->addWidget(peer_value_, 0, 3);
    grid->addWidget(session_label, 0, 4);
    grid->addWidget(session_id_value_, 0, 5);

    grid->addWidget(remote_label, 1, 0);
    grid->addWidget(remote_value_, 1, 1, 1, 3);
    grid->addWidget(listen_label, 1, 4);
    grid->addWidget(tcp_listen_value_, 1, 5);

    grid->addWidget(authority_label, 2, 0);
    grid->addWidget(authority_value_, 2, 1);
    grid->addWidget(note_label, 2, 2);
    grid->addWidget(note_value_, 2, 3, 1, 3);
    grid->addWidget(error_label, 3, 0);
    grid->addWidget(error_value_, 3, 1, 1, 5);

    grid->setColumnStretch(1, 3);
    grid->setColumnStretch(3, 2);
    grid->setColumnStretch(5, 3);
    return group;
}

QWidget* MainWindow::build_command_panel(QWidget* parent) {
    auto* group = new QGroupBox("控制面板", parent);
    auto* layout = new QGridLayout(group);
    layout->setHorizontalSpacing(8);
    layout->setVerticalSpacing(8);

    takeoff_height_spin_ = make_spin(1.5, 0.1, 20.0, 0.1);
    takeoff_velocity_spin_ = make_spin(0.8, 0.1, 5.0, 0.1);
    land_velocity_spin_ = make_spin(0.5, 0.1, 5.0, 0.1);
    return_loiter_spin_ = make_spin(0.0, 0.0, 30.0, 0.5);
    point_x_spin_ = make_spin(0.0, -100.0, 100.0, 0.1);
    point_y_spin_ = make_spin(0.0, -100.0, 100.0, 0.1);
    point_z_spin_ = make_spin(1.5, -10.0, 100.0, 0.1);
    point_yaw_spin_ = make_spin(0.0, -3.14159, 3.14159, 0.1, 3);
    vel_x_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_y_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_z_spin_ = make_spin(0.0, -5.0, 5.0, 0.1);
    vel_yaw_rate_spin_ = make_spin(0.0, -3.14159, 3.14159, 0.1, 3);

    takeoff_button_ = new QPushButton("TAKEOFF", group);
    land_button_ = new QPushButton("LAND", group);
    return_button_ = new QPushButton("RETURN", group);
    point_button_ = new QPushButton("发送 MOVE_POINT", group);
    velocity_button_ = new QPushButton("发送 MOVE_VELOCITY", group);
    command_hint_label_ = new QLabel(group);
    command_hint_label_->setWordWrap(true);
    command_hint_label_->setStyleSheet("color:#56656d;");

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

    layout->addWidget(new QLabel("目标点 x/y/z/yaw(rad)"), 2, 0);
    layout->addWidget(point_x_spin_, 2, 1);
    layout->addWidget(point_y_spin_, 2, 2);
    layout->addWidget(point_z_spin_, 2, 3);
    layout->addWidget(point_yaw_spin_, 2, 4);
    layout->addWidget(point_button_, 2, 5);

    layout->addWidget(new QLabel("惯性系速度 vx/vy/vz/yaw_rate"), 3, 0);
    layout->addWidget(vel_x_spin_, 3, 1);
    layout->addWidget(vel_y_spin_, 3, 2);
    layout->addWidget(vel_z_spin_, 3, 3);
    layout->addWidget(vel_yaw_rate_spin_, 3, 4);
    layout->addWidget(velocity_button_, 3, 5);

    layout->addWidget(command_hint_label_, 4, 0, 1, 6);
    return group;
}

QWidget* MainWindow::build_command_history_panel(QWidget* parent) {
    auto* group = new QGroupBox("命令历史", parent);
    auto* layout = new QVBoxLayout(group);
    layout->setSpacing(8);

    command_history_table_ = new QTableWidget(group);
    command_history_table_->setColumnCount(7);
    command_history_table_->setHorizontalHeaderLabels(
        {"时间", "命令", "状态", "Session", "Message ID", "回执", "接纳依据"});
    command_history_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    command_history_table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    command_history_table_->setSelectionMode(QAbstractItemView::SingleSelection);
    command_history_table_->verticalHeader()->setVisible(false);
    command_history_table_->horizontalHeader()->setStretchLastSection(false);
    command_history_table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    command_history_table_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    command_history_table_->horizontalHeader()->setSectionResizeMode(6, QHeaderView::Stretch);
    command_history_table_->setWordWrap(false);
    layout->addWidget(command_history_table_);
    return group;
}

QWidget* MainWindow::build_topics_panel(QWidget* parent) {
    auto* group = new QGroupBox("YunLink 状态", parent);
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
        layout->setSpacing(0);

        auto* table = create_topic_table(page);
        topic_tables_[key] = table;
        layout->addWidget(table, 1);
        tabs->addTab(page, QString::fromStdString(it->second.title));
    }

    root->addWidget(tabs, 1);
    return group;
}

QWidget* MainWindow::build_log_panel(QWidget* parent) {
    auto* group = new QGroupBox("运行日志", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* actions = new QHBoxLayout();
    reconnect_button_ = new QPushButton("立即重连", group);
    clear_logs_button_ = new QPushButton("清空日志", group);
    actions->addWidget(reconnect_button_);
    actions->addWidget(clear_logs_button_);
    actions->addStretch(1);
    root->addLayout(actions);

    logs_ = new QPlainTextEdit(group);
    logs_->setReadOnly(true);
    logs_->setLineWrapMode(QPlainTextEdit::NoWrap);
    logs_->setStyleSheet(
        "QPlainTextEdit { background:#13211b;color:#cde9d1;border-radius:8px;padding:8px;"
        "font-family:'DejaVu Sans Mono'; }");
    root->addWidget(logs_, 1);

    connect(reconnect_button_, &QPushButton::clicked, this, [this]() {
        if (backend_ != nullptr) {
            backend_->request_reconnect_now();
        }
    });
    connect(clear_logs_button_, &QPushButton::clicked, this, [this]() {
        if (backend_ != nullptr) {
            backend_->clear_logs();
        }
        rendered_last_sequence_ = 0;
        rendered_log_count_ = 0;
        logs_->clear();
    });
    return group;
}
