#include "ui/main_window.hpp"

#include <cstdlib>
#include <sstream>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "model/comparison.hpp"
#include "model/format.hpp"
#include "model/topic_defs.hpp"

MainWindow::MainWindow(CompareBackend* backend, QWidget* parent)
    : QMainWindow(parent), backend_(backend), align_window_ms_(backend->align_window_ms()) {
    setWindowTitle("Sunray 与 Yunlink 数据一致性对比工具");
    resize(1880, 1020);
    build_ui();

    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &MainWindow::refresh_view);
    timer->start(250);
}

void MainWindow::build_ui() {
    auto* central = new QWidget(this);
    auto* root_layout = new QVBoxLayout(central);
    root_layout->setContentsMargins(14, 14, 14, 14);
    root_layout->setSpacing(10);

    auto* title = new QLabel("Sunray 与 Yunlink 数据一致性对比工具", central);
    QFont title_font("DejaVu Sans", 18, QFont::Bold);
    title->setFont(title_font);
    title->setStyleSheet("color:#173127;");
    root_layout->addWidget(title);

    summary_label_ = new QLabel(central);
    summary_label_->setWordWrap(true);
    summary_label_->setStyleSheet(
        "background:#eff6ef;color:#284033;border:1px solid #b6cdb8;border-radius:10px;"
        "padding:10px;");
    root_layout->addWidget(summary_label_);

    auto* tabs = new QTabWidget(central);
    tabs->setStyleSheet("QTabBar::tab { background:#d7e6da; padding:8px 14px; }"
                        "QTabBar::tab:selected { background:#f2f6f0; }");
    root_layout->addWidget(tabs, 1);

    const auto topics = backend_->snapshot_topics();
    for (const auto& key : topic_display_order()) {
        auto* page = new QWidget(tabs);
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);

        auto* info = new QLabel(page);
        info->setWordWrap(true);
        info->setStyleSheet(
            "background:#f7faf6;color:#304137;border:1px solid #c3d3c5;border-radius:8px;"
            "padding:8px;");
        topic_info_[key] = info;
        layout->addWidget(info);

        auto* compare_tabs = new QTabWidget(page);
        compare_tabs->setStyleSheet("QTabBar::tab { background:#e5efe5; padding:6px 12px; }"
                                    "QTabBar::tab:selected { background:#fbfdfb; }");

        auto* latest_page = new QWidget(compare_tabs);
        auto* latest_layout = new QVBoxLayout(latest_page);
        latest_layout->setContentsMargins(0, 0, 0, 0);
        auto* latest_table = create_compare_table(
            latest_page, {"字段说明", "ROS 最新值", "Yunlink 最新值", "差值", "比对结果"});
        topic_latest_tables_[key] = latest_table;
        latest_layout->addWidget(latest_table);
        compare_tabs->addTab(latest_page, "最新值直比");

        auto* aligned_page = new QWidget(compare_tabs);
        auto* aligned_layout = new QVBoxLayout(aligned_page);
        aligned_layout->setContentsMargins(0, 0, 0, 0);
        auto* aligned_table = create_compare_table(
            aligned_page, {"字段说明", "ROS 对齐值", "Yunlink 对齐值", "差值", "比对结果"});
        topic_aligned_tables_[key] = aligned_table;
        aligned_layout->addWidget(aligned_table);
        compare_tabs->addTab(aligned_page, "时间对齐比");

        layout->addWidget(compare_tabs, 1);

        auto* uncovered = new QLabel(page);
        uncovered->setWordWrap(true);
        uncovered->setStyleSheet("color:#5a5f48;");
        uncovered_labels_[key] = uncovered;
        layout->addWidget(uncovered);

        tabs->addTab(page, QString::fromStdString(topics.at(key).title));
    }

    logs_ = new QTextEdit(central);
    logs_->setReadOnly(true);
    logs_->setMinimumHeight(140);
    logs_->setStyleSheet(
        "QTextEdit { background:#13211b;color:#cde9d1;border-radius:8px;padding:6px;"
        "font-family:'DejaVu Sans Mono'; }");
    root_layout->addWidget(logs_);

    setCentralWidget(central);
}

void MainWindow::refresh_view() {
    const auto topics = backend_->snapshot_topics();
    const auto logs = backend_->snapshot_logs();

    std::ostringstream summary;
    summary << "范围：local_odom、odom_state、uav_control_state、mavros_state、px4_state\n";
    summary << "模式：最新值直比 | 时间对齐比（窗口 <= " << fmt_ms(align_window_ms_)
            << " ms，超出窗口时保留最近样本）";
    summary_label_->setText(QString::fromStdString(summary.str()));

    for (const auto& item : topics) {
        refresh_topic(item.first, item.second);
    }

    std::ostringstream log_ss;
    for (const auto& line : logs) {
        log_ss << line << "\n";
    }
    logs_->setPlainText(QString::fromStdString(log_ss.str()));
}

void MainWindow::refresh_topic(const std::string& key, const TopicState& topic) {
    auto* info = topic_info_[key];
    auto* uncovered = uncovered_labels_[key];
    auto* latest_table = topic_latest_tables_[key];
    auto* aligned_table = topic_aligned_tables_[key];
    const ComparisonSelection latest_selection = make_latest_selection(topic);
    const ComparisonSelection aligned_selection = make_aligned_selection(topic, align_window_ms_);

    std::ostringstream info_ss;
    info_ss << "ROS: " << topic.ros_topic << "\n";
    info_ss << "Yunlink: " << topic.yunlink_name << "\n";
    info_ss << "ROS stamp " << fmt_ros_time(topic.ros.msg_stamp) << " | ROS rx "
            << fmt_ros_time(topic.ros.receive_time) << "\n";
    info_ss << "Yunlink rx " << fmt_ros_time(topic.yunlink.receive_time);
    if (topic.yunlink.message_id != 0) {
        info_ss << " | msg_id " << topic.yunlink.message_id;
    }
    if (!topic.yunlink.note.empty()) {
        info_ss << " | " << topic.yunlink.note;
    }
    info_ss << "\n最新 dt "
            << (latest_selection.matched ? fmt_ms(latest_selection.receive_dt_ms) : "--") << " ms";
    if (aligned_selection.matched) {
        if (aligned_selection.within_align_window) {
            info_ss << " | 对齐 dt " << fmt_ms(aligned_selection.receive_dt_ms) << " ms";
        } else {
            info_ss << " | 最近 dt " << fmt_ms(aligned_selection.receive_dt_ms) << " ms（超过窗口）";
        }
    } else if (has_snapshot(aligned_selection.yunlink)) {
        info_ss << " | 对齐 dt > " << fmt_ms(align_window_ms_) << " ms";
    } else {
        info_ss << " | 对齐 dt --";
    }
    info->setText(QString::fromStdString(info_ss.str()));

    populate_compare_table(latest_table, topic, latest_selection);
    populate_compare_table(aligned_table, topic, aligned_selection);

    if (topic.uncovered_fields.empty()) {
        uncovered->clear();
    } else {
        std::ostringstream uncovered_ss;
        uncovered_ss << "未覆盖字段：";
        for (size_t i = 0; i < topic.uncovered_fields.size(); ++i) {
            if (i > 0) {
                uncovered_ss << ", ";
            }
            uncovered_ss << topic.uncovered_fields[i];
        }
        uncovered->setText(QString::fromStdString(uncovered_ss.str()));
    }
}

QTableWidget* MainWindow::create_compare_table(QWidget* parent, const QStringList& headers) {
    auto* table = new QTableWidget(parent);
    table->setColumnCount(headers.size());
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(false);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    table->setAlternatingRowColors(true);
    table->setWordWrap(false);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setStyleSheet("QTableWidget { background:#fbfdfb; alternate-background-color:#f1f6f1; }");
    return table;
}

void MainWindow::populate_compare_table(QTableWidget* table,
                                        const TopicState& topic,
                                        const ComparisonSelection& selection) {
    table->setRowCount(static_cast<int>(topic.rows.size()));
    for (int row = 0; row < static_cast<int>(topic.rows.size()); ++row) {
        const auto& template_row = topic.rows[row];
        const std::string& key_name = template_row.key;
        const auto ros_it = selection.ros.values.find(key_name);
        const auto yn_it = selection.yunlink.values.find(key_name);
        const std::string ros_value = ros_it == selection.ros.values.end() ? "--" : ros_it->second;
        const std::string yn_value =
            yn_it == selection.yunlink.values.end() ? "--" : yn_it->second;
        const bool has_both =
            ros_it != selection.ros.values.end() && yn_it != selection.yunlink.values.end();
        const bool numeric = has_both && is_numeric(ros_value) && is_numeric(yn_value);
        const bool equal =
            has_both && (numeric ? equal_float(ros_value, yn_value, field_epsilon(topic.key, key_name))
                                 : equal_text(ros_value, yn_value));
        const std::string delta = numeric ? delta_float(ros_value, yn_value) : "--";
        const std::string match = has_both ? (equal ? "OK" : "DIFF") : "WAIT";

        set_item(table, row, 0, template_row.label);
        set_item(table, row, 1, ros_value);
        set_item(table, row, 2, yn_value);
        set_item(table, row, 3, delta);
        auto* match_item = set_item(table, row, 4, match);
        table->item(row, 0)->setToolTip(QString::fromStdString(template_row.label));
        table->item(row, 1)->setToolTip(QString::fromStdString(ros_value));
        table->item(row, 2)->setToolTip(QString::fromStdString(yn_value));
        if (match == "OK") {
            match_item->setBackground(QBrush(QColor("#d7f0d6")));
        } else if (match == "DIFF") {
            match_item->setBackground(QBrush(QColor("#f7d7d7")));
        } else {
            match_item->setBackground(QBrush(QColor("#efe8c8")));
        }
    }
}

bool MainWindow::is_numeric(const std::string& value) {
    if (value.empty() || value == "--") {
        return false;
    }
    char* end = nullptr;
    std::strtod(value.c_str(), &end);
    return end != nullptr && *end == '\0';
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
    item->setTextAlignment((col == 3 || col == 4) ? Qt::AlignCenter
                                                  : (Qt::AlignLeft | Qt::AlignVCenter));
    return item;
}
