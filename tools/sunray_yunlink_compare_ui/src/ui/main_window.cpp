#include "ui/main_window.hpp"

#include <cstdlib>
#include <ctime>
#include <sstream>

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFileDialog>
#include <QFile>
#include <QFont>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QScrollBar>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTabWidget>
#include <QTextBlock>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include "model/comparison.hpp"
#include "model/format.hpp"
#include "model/topic_defs.hpp"

namespace {

class LogViewHighlighter : public QSyntaxHighlighter {
  public:
    explicit LogViewHighlighter(QTextDocument* parent = nullptr) : QSyntaxHighlighter(parent) {
        info_format_.setForeground(QColor("#cde9d1"));
        warning_format_.setForeground(QColor("#f6d06d"));
        warning_format_.setFontWeight(QFont::DemiBold);
        error_format_.setForeground(QColor("#ff8e8e"));
        error_format_.setFontWeight(QFont::Bold);
    }

  protected:
    void highlightBlock(const QString& text) override {
        if (text.contains("[ERROR]")) {
            setFormat(0, text.size(), error_format_);
            return;
        }
        if (text.contains("[WARNING]")) {
            setFormat(0, text.size(), warning_format_);
            return;
        }
        setFormat(0, text.size(), info_format_);
    }

  private:
    QTextCharFormat info_format_;
    QTextCharFormat warning_format_;
    QTextCharFormat error_format_;
};

}  // namespace

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

    auto* vertical_splitter = new QSplitter(Qt::Vertical, central);
    vertical_splitter->setChildrenCollapsible(false);
    vertical_splitter->setHandleWidth(10);

    auto* main_area = new QWidget(vertical_splitter);
    auto* main_layout = new QVBoxLayout(main_area);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    auto* tabs = new QTabWidget(central);
    tabs->setStyleSheet("QTabBar::tab { background:#d7e6da; padding:8px 14px; }"
                        "QTabBar::tab:selected { background:#f2f6f0; }");
    main_layout->addWidget(tabs, 1);

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

    vertical_splitter->addWidget(main_area);
    vertical_splitter->addWidget(build_log_panel(vertical_splitter));
    vertical_splitter->setStretchFactor(0, 8);
    vertical_splitter->setStretchFactor(1, 2);
    vertical_splitter->setSizes({760, 260});
    root_layout->addWidget(vertical_splitter, 1);

    setCentralWidget(central);
    setStyleSheet(styleSheet() +
                  "QSplitter::handle:vertical { background:#d1ddd0; border-radius:4px; }"
                  "QSplitter::handle:vertical:hover { background:#b9cab9; }");
}

QWidget* MainWindow::build_log_panel(QWidget* parent) {
    auto* panel = new QWidget(parent);
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    auto* title_row = new QHBoxLayout();
    auto* title = new QLabel("操作日志", panel);
    QFont title_font("DejaVu Sans", 12, QFont::Bold);
    title->setFont(title_font);
    title->setStyleSheet("color:#173127;");
    title_row->addWidget(title);
    title_row->addStretch(1);
    log_status_label_ = new QLabel("等待日志", panel);
    log_status_label_->setStyleSheet("color:#567061;");
    title_row->addWidget(log_status_label_);
    layout->addLayout(title_row);

    auto* controls_row = new QHBoxLayout();
    controls_row->setSpacing(8);
    follow_latest_checkbox_ = new QCheckBox("跟随最新", panel);
    follow_latest_checkbox_->setChecked(true);
    pause_logs_checkbox_ = new QCheckBox("暂停刷新", panel);
    clear_logs_button_ = new QPushButton("清空", panel);
    copy_selected_button_ = new QPushButton("复制选中", panel);
    export_logs_button_ = new QPushButton("导出日志", panel);
    jump_to_latest_button_ = new QPushButton("查看新日志", panel);
    jump_to_latest_button_->setVisible(false);

    controls_row->addWidget(follow_latest_checkbox_);
    controls_row->addWidget(pause_logs_checkbox_);
    controls_row->addWidget(clear_logs_button_);
    controls_row->addWidget(copy_selected_button_);
    controls_row->addWidget(export_logs_button_);
    controls_row->addStretch(1);
    controls_row->addWidget(jump_to_latest_button_);
    layout->addLayout(controls_row);

    logs_ = new QPlainTextEdit(panel);
    logs_->setReadOnly(true);
    logs_->setMinimumHeight(160);
    logs_->setLineWrapMode(QPlainTextEdit::NoWrap);
    logs_->setStyleSheet(
        "QPlainTextEdit { background:#13211b;color:#cde9d1;border-radius:8px;padding:8px;"
        "font-family:'DejaVu Sans Mono'; selection-background-color:#2b5c49; }");
    new LogViewHighlighter(logs_->document());
    layout->addWidget(logs_, 1);

    connect(logs_->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        if (suppress_log_scroll_events_) {
            return;
        }
        if (is_log_view_at_bottom() && pending_new_log_count_ > 0) {
            pending_new_log_count_ = 0;
            jump_to_latest_button_->setVisible(false);
            sync_log_status();
        }
    });

    connect(jump_to_latest_button_, &QPushButton::clicked, this, [this]() {
        pause_logs_checkbox_->setChecked(false);
        follow_latest_checkbox_->setChecked(true);
        pending_new_log_count_ = 0;
        jump_to_latest_button_->setVisible(false);
        rebuild_log_view_from_entries(backend_->snapshot_logs());
        sync_log_status();
    });

    connect(clear_logs_button_, &QPushButton::clicked, this, [this]() {
        backend_->clear_logs();
        pending_new_log_count_ = 0;
        log_overflow_since_render_ = false;
        jump_to_latest_button_->setVisible(false);
        reset_log_view();
        sync_log_status();
    });

    connect(copy_selected_button_, &QPushButton::clicked, this, [this]() { copy_selected_logs(); });
    connect(export_logs_button_, &QPushButton::clicked, this, [this]() { export_logs(); });
    connect(follow_latest_checkbox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (checked && !pause_logs_checkbox_->isChecked()) {
            pending_new_log_count_ = 0;
            jump_to_latest_button_->setVisible(false);
            scroll_logs_to_bottom();
            sync_log_status();
        }
    });
    connect(pause_logs_checkbox_, &QCheckBox::toggled, this, [this](bool checked) {
        if (!checked) {
            rebuild_log_view_from_entries(backend_->snapshot_logs());
        }
        sync_log_status();
    });

    return panel;
}
