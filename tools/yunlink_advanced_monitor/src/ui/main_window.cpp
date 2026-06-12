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
    setWindowTitle("yunlink_advanced_monitor");
    resize(1320, 820);
    build_ui();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh_view);
    timer->start(500);
    refresh_view();
}

QTableWidget* MainWindow::create_topic_table(QWidget* parent) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"字段说明", "YunLink 最新值"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setWordWrap(false);
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

    auto* top_grid = new QGridLayout();
    top_grid->setHorizontalSpacing(12);
    top_grid->setVerticalSpacing(12);
    top_grid->addWidget(
        build_dashboard_card(content, "YunLink", &dashboard_yunlink_summary_, &dashboard_yunlink_body_),
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
                                             "Command",
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

    auto* issues_group = new QGroupBox("Developer Issues", content);
    auto* issues_layout = new QVBoxLayout(issues_group);
    issues_layout->setContentsMargins(12, 12, 12, 12);
    dashboard_issues_value_ = new QLabel(issues_group);
    dashboard_issues_value_->setWordWrap(true);
    dashboard_issues_value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    issues_layout->addWidget(dashboard_issues_value_);

    root->addLayout(top_grid);
    root->addLayout(lower_grid);
    root->addWidget(issues_group);
    root->addStretch(1);

    scroll->setWidget(content);
    return scroll;
}
