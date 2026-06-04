#include "ui/main_window.hpp"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QTimer>

MainWindow::MainWindow(AdvancedMonitorBackend* backend, QWidget* parent)
    : QMainWindow(parent), backend_(backend) {
    setWindowTitle("sunray_advanced_monitor");
    resize(1320, 820);
    build_ui();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh_view);
    timer->start(500);
}

QTableWidget* MainWindow::create_topic_table(QWidget* parent) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"字段说明", "ROS 最新值", "YunLink 最新值", "时差/异常"});
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->verticalHeader()->setVisible(false);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
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
