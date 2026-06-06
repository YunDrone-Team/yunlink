#include "ui/main_window.hpp"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QSplitter>
#include <QStringList>
#include <QVBoxLayout>
#include <QCheckBox>

namespace {

std::string bool_label(bool value) {
    return value ? "true" : "false";
}

std::string join_values(const std::vector<std::string>& values, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < values.size(); ++i) {
        if (i != 0) {
            out += sep;
        }
        out += values[i];
    }
    return out;
}

std::vector<std::string> split_override_args(const std::string& text) {
    std::vector<std::string> values;
    std::string current;
    for (char ch : text) {
        if (ch == ',') {
            if (!current.empty()) {
                size_t begin = current.find_first_not_of(" \t\r\n");
                size_t end = current.find_last_not_of(" \t\r\n");
                if (begin != std::string::npos) {
                    values.push_back(current.substr(begin, end - begin + 1));
                }
            }
            current.clear();
            continue;
        }
        current.push_back(ch);
    }
    if (!current.empty()) {
        size_t begin = current.find_first_not_of(" \t\r\n");
        size_t end = current.find_last_not_of(" \t\r\n");
        if (begin != std::string::npos) {
            values.push_back(current.substr(begin, end - begin + 1));
        }
    }
    return values;
}

}  // namespace

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
    backend_->request_feature_stop(feature_name_edit_->text().trimmed().toStdString(),
                                   feature_force_stop_checkbox_ != nullptr &&
                                       feature_force_stop_checkbox_->isChecked());
}

QWidget* MainWindow::build_system_service_panel(QWidget* parent) {
    auto* group = new QGroupBox("System Service", parent);
    auto* root = new QVBoxLayout(group);
    root->setSpacing(8);

    auto* action_row = new QHBoxLayout();
    refresh_feature_list_button_ = new QPushButton("刷新 FeatureList", group);
    feature_name_edit_ = new QLineEdit(group);
    feature_name_edit_->setPlaceholderText("feature_name");
    refresh_feature_detail_button_ = new QPushButton("查询 FeatureGet", group);
    action_row->addWidget(refresh_feature_list_button_);
    action_row->addWidget(feature_name_edit_, 1);
    action_row->addWidget(refresh_feature_detail_button_);
    root->addLayout(action_row);

    auto* control_row = new QHBoxLayout();
    feature_override_args_edit_ = new QLineEdit(group);
    feature_override_args_edit_->setPlaceholderText("override_args, comma separated");
    feature_restart_checkbox_ = new QCheckBox("restart_if_running", group);
    feature_terminal_checkbox_ = new QCheckBox("start_with_terminal", group);
    feature_force_stop_checkbox_ = new QCheckBox("force_stop", group);
    start_feature_button_ = new QPushButton("发送 FeatureStart", group);
    stop_feature_button_ = new QPushButton("发送 FeatureStop", group);
    control_row->addWidget(feature_override_args_edit_, 1);
    control_row->addWidget(feature_restart_checkbox_);
    control_row->addWidget(feature_terminal_checkbox_);
    control_row->addWidget(feature_force_stop_checkbox_);
    control_row->addWidget(start_feature_button_);
    control_row->addWidget(stop_feature_button_);
    root->addLayout(control_row);

    feature_list_text_ = new QPlainTextEdit(group);
    feature_list_text_->setReadOnly(true);
    feature_list_text_->setMaximumBlockCount(200);
    feature_detail_text_ = new QPlainTextEdit(group);
    feature_detail_text_->setReadOnly(true);
    feature_detail_text_->setMaximumBlockCount(400);

    system_service_history_table_ = new QTableWidget(group);
    system_service_history_table_->setColumnCount(6);
    system_service_history_table_->setHorizontalHeaderLabels(
        {"时间", "请求", "Feature", "状态", "Session", "结果"});
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

    auto* content = new QSplitter(Qt::Horizontal, group);
    auto* left = new QWidget(content);
    auto* left_layout = new QVBoxLayout(left);
    left_layout->setContentsMargins(0, 0, 0, 0);
    left_layout->addWidget(new QLabel("FeatureList", left));
    left_layout->addWidget(feature_list_text_, 1);
    left_layout->addWidget(new QLabel("请求历史", left));
    left_layout->addWidget(system_service_history_table_, 1);

    auto* right = new QWidget(content);
    auto* right_layout = new QVBoxLayout(right);
    right_layout->setContentsMargins(0, 0, 0, 0);
    right_layout->addWidget(new QLabel("FeatureGet 详情", right));
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
    connect(start_feature_button_, &QPushButton::clicked, this, &MainWindow::stage_start_feature);
    connect(stop_feature_button_, &QPushButton::clicked, this, &MainWindow::stage_stop_feature);
    return group;
}

void MainWindow::refresh_system_services() {
    if (backend_ == nullptr || feature_list_text_ == nullptr || feature_detail_text_ == nullptr ||
        system_service_history_table_ == nullptr) {
        return;
    }

    const auto state = backend_->snapshot_system_services();
    const auto history = backend_->snapshot_system_service_history();

    QStringList feature_lines;
    if (!state.last_status.empty()) {
        feature_lines.append(QString::fromStdString("status: " + state.last_status));
    }
    for (const auto& name : state.feature_names) {
        feature_lines.append(QString::fromStdString(name));
    }
    feature_list_text_->setPlainText(feature_lines.join('\n'));

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
    feature_detail_text_->setPlainText(detail_lines.join('\n'));

    system_service_history_table_->setRowCount(static_cast<int>(history.size()));
    for (int row = 0; row < static_cast<int>(history.size()); ++row) {
        const auto& entry = history[history.size() - 1 - static_cast<size_t>(row)];
        set_item(system_service_history_table_, row, 0, format_timestamp(entry.sent_at_ms).toStdString());
        set_item(system_service_history_table_, row, 1, entry.action);
        set_item(system_service_history_table_, row, 2, entry.feature_name.empty() ? "--" : entry.feature_name);
        set_item(system_service_history_table_,
                 row,
                 3,
                 system_service_lifecycle_label(entry.lifecycle));
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
