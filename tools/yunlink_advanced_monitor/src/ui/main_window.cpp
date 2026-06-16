#include "ui/main_window.hpp"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QFrame>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QTimer>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"

QWidget* MainWindow::build_dashboard_card(QWidget* parent,
                                          const QString& title,
                                          QLabel** summary,
                                          QLabel** body) {
    auto* group = new QGroupBox(title, parent);
    group->setStyleSheet("QGroupBox { font-weight:600; }");
    auto* layout = new QVBoxLayout(group);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* summary_label = new QLabel(group);
    summary_label->setWordWrap(true);
    summary_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    auto* body_label = new QLabel(group);
    body_label->setWordWrap(true);
    body_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    body_label->setStyleSheet("color:#334155;");

    layout->addWidget(summary_label);
    layout->addWidget(body_label);
    layout->addStretch(1);

    if (summary != nullptr) {
        *summary = summary_label;
    }
    if (body != nullptr) {
        *body = body_label;
    }
    return group;
}

MainWindow::MainWindow(AdvancedMonitorBackend* backend, QWidget* parent)
    : QMainWindow(parent), backend_(backend) {
    setWindowTitle("YunLink Advanced Monitor");
    resize(1320, 820);
    build_ui();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh_view);
    timer->start(500);
    refresh_view();
}

QTableWidget* MainWindow::create_topic_table(QWidget* parent) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(3);
    table->setHorizontalHeaderLabels({"字段说明", "Field key", "YunLink 最新值"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->setWordWrap(false);
    monitor_ui::style_table(table);
    return table;
}

QTableWidgetItem* MainWindow::set_item(QTableWidget* table,
                                       int row,
                                       int col,
                                       const std::string& text) {
    auto* item = table->item(row, col);
    if (item == nullptr) {
        item = new QTableWidgetItem();
        table->setItem(row, col, item);
    }
    item->setText(QString::fromStdString(text));
    monitor_ui::style_item(item, col > 0);
    item->setBackground(QBrush());
    item->setForeground(QBrush());
    return item;
}

QDoubleSpinBox* MainWindow::make_spin(double value,
                                      double min_value,
                                      double max_value,
                                      double step,
                                      int decimals) {
    auto* spin = new QDoubleSpinBox();
    spin->setRange(min_value, max_value);
    spin->setDecimals(decimals);
    spin->setSingleStep(step);
    spin->setValue(value);
    return spin;
}

QString MainWindow::format_timestamp(uint64_t timestamp_ms) {
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(timestamp_ms))
        .toString("HH:mm:ss.zzz");
}

QWidget* MainWindow::build_dashboard_page(QWidget* parent) {
    auto* scroll = new QScrollArea(parent);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);

    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(12);

    auto* status_group = new QGroupBox("Operations status strip", content);
    auto* status_grid = new QGridLayout(status_group);
    status_grid->setContentsMargins(12, 12, 12, 12);
    status_grid->setHorizontalSpacing(12);
    status_grid->setVerticalSpacing(8);

    status_value_ = new QLabel(status_group);
    peer_value_ = new QLabel(status_group);
    session_id_value_ = new QLabel(status_group);
    remote_value_ = new QLabel(status_group);
    tcp_listen_value_ = new QLabel(status_group);
    authority_value_ = new QLabel(status_group);
    note_value_ = new QLabel(status_group);
    error_value_ = new QLabel(status_group);
    monitor_ui::set_mono(peer_value_);
    monitor_ui::set_mono(session_id_value_);
    monitor_ui::set_mono(remote_value_);
    monitor_ui::set_mono(tcp_listen_value_);
    note_value_->setWordWrap(true);
    error_value_->setWordWrap(true);

    status_grid->addWidget(new QLabel("链路 / session", status_group), 0, 0);
    status_grid->addWidget(status_value_, 0, 1);
    status_grid->addWidget(new QLabel("authority", status_group), 0, 2);
    status_grid->addWidget(authority_value_, 0, 3);
    status_grid->addWidget(new QLabel("peer id", status_group), 1, 0);
    status_grid->addWidget(peer_value_, 1, 1);
    status_grid->addWidget(new QLabel("session id", status_group), 1, 2);
    status_grid->addWidget(session_id_value_, 1, 3);
    status_grid->addWidget(new QLabel("remote", status_group), 2, 0);
    status_grid->addWidget(remote_value_, 2, 1);
    status_grid->addWidget(new QLabel("listen", status_group), 2, 2);
    status_grid->addWidget(tcp_listen_value_, 2, 3);
    status_grid->addWidget(new QLabel("最近说明", status_group), 3, 0);
    status_grid->addWidget(note_value_, 3, 1);
    status_grid->addWidget(new QLabel("最近错误", status_group), 3, 2);
    status_grid->addWidget(error_value_, 3, 3);
    status_grid->setColumnStretch(1, 2);
    status_grid->setColumnStretch(3, 2);

    auto* top_grid = new QGridLayout();
    top_grid->setHorizontalSpacing(12);
    top_grid->setVerticalSpacing(12);
    top_grid->addWidget(
        build_dashboard_card(content, "YunLink link", &dashboard_yunlink_summary_, &dashboard_yunlink_body_),
        0,
        0);
    top_grid->addWidget(
        build_dashboard_card(content, "PX4", &dashboard_px4_summary_, &dashboard_px4_body_), 0, 1);
    top_grid->addWidget(build_dashboard_card(content,
                                             "Localization",
                                             &dashboard_localization_summary_,
                                             &dashboard_localization_body_),
                        0,
                        2);
    top_grid->addWidget(build_dashboard_card(content,
                                             "Control",
                                             &dashboard_control_summary_,
                                             &dashboard_control_body_),
                        0,
                        3);
    top_grid->addWidget(build_dashboard_card(content,
                                             "Command gate",
                                             &dashboard_command_summary_,
                                             &dashboard_command_body_),
                        0,
                        4);
    for (int col = 0; col < 5; ++col) {
        top_grid->setColumnStretch(col, 1);
    }

    auto* lower_grid = new QGridLayout();
    lower_grid->setHorizontalSpacing(12);
    lower_grid->setVerticalSpacing(12);

    auto* control_group = new QGroupBox("UAV 控制状态面板", content);
    auto* control_layout = new QVBoxLayout(control_group);
    control_layout->setContentsMargins(12, 12, 12, 12);
    auto* control_label = new QLabel(control_group);
    control_label->setWordWrap(true);
    control_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dashboard_control_panel_ = control_label;
    control_layout->addWidget(control_label);

    auto* localization_group = new QGroupBox("Localization Fusion 状态面板", content);
    auto* localization_layout = new QVBoxLayout(localization_group);
    localization_layout->setContentsMargins(12, 12, 12, 12);
    auto* localization_label = new QLabel(localization_group);
    localization_label->setWordWrap(true);
    localization_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    dashboard_localization_panel_ = localization_label;
    localization_layout->addWidget(localization_label);

    lower_grid->addWidget(control_group, 0, 0);
    lower_grid->addWidget(localization_group, 0, 1);
    lower_grid->setColumnStretch(0, 1);
    lower_grid->setColumnStretch(1, 1);

    auto* issues_group = new QGroupBox("Blocking issues", content);
    auto* issues_layout = new QVBoxLayout(issues_group);
    issues_layout->setContentsMargins(12, 12, 12, 12);
    dashboard_issues_value_ = new QLabel(issues_group);
    dashboard_issues_value_->setWordWrap(true);
    dashboard_issues_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    issues_layout->addWidget(dashboard_issues_value_);

    root->addWidget(status_group);
    root->addLayout(top_grid);
    root->addLayout(lower_grid);
    root->addWidget(issues_group);
    root->addStretch(1);

    scroll->setWidget(content);
    return scroll;
}
