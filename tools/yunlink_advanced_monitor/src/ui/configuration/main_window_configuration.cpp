#include "ui/main_window.hpp"

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <sstream>
#include <system_error>

#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "common/monitor_ui_style.hpp"

namespace {

std::vector<std::string> split_string_list(const QString& text) {
    std::vector<std::string> result;
    for (const QString& item : text.split(';', Qt::SkipEmptyParts)) {
        const QString normalized = item.trimmed();
        if (!normalized.isEmpty()) {
            result.push_back(normalized.toStdString());
        }
    }
    return result;
}

yunlink::ConfigValue int64_value_from_text(const QString& text) {
    const std::string value = text.trimmed().toStdString();
    int64_t parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    return result.ec == std::errc() && result.ptr == value.data() + value.size()
               ? yunlink::ConfigValue::from_int64(parsed)
               : yunlink::ConfigValue::from_string(value);
}

yunlink::ConfigValue double_value_from_text(const QString& text) {
    bool valid = false;
    const double parsed = text.trimmed().toDouble(&valid);
    return valid ? yunlink::ConfigValue::from_double(parsed)
                 : yunlink::ConfigValue::from_string(text.trimmed().toStdString());
}

}  // namespace

QWidget* MainWindow::build_configuration_page(QWidget* parent) {
    auto* page = new QWidget(parent);
    auto* root = new QVBoxLayout(page);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    config_resource_combo_ = new QComboBox(page);
    config_resource_combo_->setMinimumWidth(280);
    config_refresh_button_ = new QPushButton("刷新", page);
    config_validate_button_ = new QPushButton("校验", page);
    config_save_button_ = new QPushButton("保存", page);
    config_apply_button_ = new QPushButton("生效", page);
    monitor_ui::style_button(config_refresh_button_, monitor_ui::ButtonRole::kSecondary);
    monitor_ui::style_button(config_validate_button_, monitor_ui::ButtonRole::kSecondary);
    monitor_ui::style_button(config_save_button_, monitor_ui::ButtonRole::kPrimary);
    monitor_ui::style_button(config_apply_button_, monitor_ui::ButtonRole::kWarning);
    toolbar->addWidget(new QLabel("配置资源", page));
    toolbar->addWidget(config_resource_combo_, 1);
    toolbar->addWidget(config_refresh_button_);
    toolbar->addWidget(config_validate_button_);
    toolbar->addWidget(config_save_button_);
    toolbar->addWidget(config_apply_button_);
    root->addLayout(toolbar);

    config_status_label_ = new QLabel("等待配置资源", page);
    config_status_label_->setWordWrap(true);
    config_status_label_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    root->addWidget(config_status_label_);

    config_fields_table_ = new QTableWidget(page);
    config_fields_table_->setColumnCount(3);
    config_fields_table_->setHorizontalHeaderLabels({"字段", "值", "说明"});
    config_fields_table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    config_fields_table_->setSelectionMode(QAbstractItemView::NoSelection);
    config_fields_table_->verticalHeader()->setVisible(false);
    config_fields_table_->horizontalHeader()->setSectionResizeMode(0,
                                                                   QHeaderView::ResizeToContents);
    config_fields_table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    config_fields_table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    monitor_ui::style_table(config_fields_table_);
    root->addWidget(config_fields_table_, 1);

    connect(config_refresh_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_refresh_configuration);
    connect(config_validate_button_,
            &QPushButton::clicked,
            this,
            &MainWindow::stage_validate_configuration);
    connect(
        config_save_button_, &QPushButton::clicked, this, &MainWindow::stage_save_configuration);
    connect(
        config_apply_button_, &QPushButton::clicked, this, &MainWindow::stage_apply_configuration);
    connect(config_resource_combo_,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) {
                config_editor_resource_id_.clear();
                config_loaded_revision_.clear();
                refresh_configuration();
            });
    return page;
}

void MainWindow::stage_refresh_configuration() {
    if (backend_ == nullptr) {
        return;
    }
    backend_->request_config_resource_list();
    const std::string id = config_resource_combo_ == nullptr
                               ? std::string()
                               : config_resource_combo_->currentData().toString().toStdString();
    if (!id.empty()) {
        backend_->request_config_resource_describe(id);
        backend_->request_config_resource_get(id);
    }
}

void MainWindow::stage_validate_configuration() {
    if (backend_ == nullptr || config_editor_resource_id_.empty() ||
        config_loaded_revision_.empty()) {
        return;
    }
    backend_->request_config_resource_patch(
        config_editor_resource_id_, config_loaded_revision_, collect_configuration_updates(), true);
}

void MainWindow::stage_save_configuration() {
    if (backend_ == nullptr || config_editor_resource_id_.empty() ||
        config_loaded_revision_.empty()) {
        return;
    }
    if (!monitor_ui::confirm_warning(
            this, "确认保存设备配置", "配置将写入远端设备，但不会立即重启或生效。")) {
        return;
    }
    backend_->request_config_resource_patch(config_editor_resource_id_,
                                            config_loaded_revision_,
                                            collect_configuration_updates(),
                                            false);
}

void MainWindow::stage_apply_configuration() {
    if (backend_ == nullptr || config_editor_resource_id_.empty() ||
        config_loaded_revision_.empty()) {
        return;
    }
    if (!monitor_ui::confirm_warning(
            this, "确认应用设备配置", "应用身份配置可能重启远端控制栈并暂时断开当前连接。")) {
        return;
    }
    backend_->request_config_resource_apply(config_editor_resource_id_, config_loaded_revision_);
}

void MainWindow::rebuild_configuration_editors(const MonitorConfigurationResourceState& resource) {
    config_fields_table_->setRowCount(static_cast<int>(resource.fields.size()));
    config_editors_.clear();
    config_editor_schema_ = resource.fields;
    for (std::size_t index = 0; index < resource.fields.size(); ++index) {
        const auto& field = resource.fields[index];
        const int row = static_cast<int>(index);
        auto* name = new QTableWidgetItem(QString::fromStdString(field.title));
        name->setToolTip(QString::fromStdString(field.path));
        config_fields_table_->setItem(row, 0, name);
        config_fields_table_->setItem(
            row, 2, new QTableWidgetItem(QString::fromStdString(field.description)));

        QWidget* editor = nullptr;
        if (!field.choices.empty()) {
            auto* combo = new QComboBox(config_fields_table_);
            for (const auto& choice : field.choices) {
                combo->addItem(QString::fromStdString(choice.label));
            }
            editor = combo;
        } else if (field.type == yunlink::ConfigValueType::kBool) {
            editor = new QCheckBox(config_fields_table_);
        } else {
            editor = new QLineEdit(config_fields_table_);
        }
        if (field.sensitive) {
            if (auto* line_edit = qobject_cast<QLineEdit*>(editor)) {
                line_edit->setEchoMode(QLineEdit::Password);
            }
        }
        editor->setEnabled(!field.read_only);
        config_fields_table_->setCellWidget(row, 1, editor);
        config_editors_[field.path] = editor;
    }
}

std::vector<yunlink::ConfigFieldValue> MainWindow::collect_configuration_updates() const {
    std::vector<yunlink::ConfigFieldValue> updates;
    for (const auto& field : config_editor_schema_) {
        if (field.read_only) {
            continue;
        }
        const auto editor_it = config_editors_.find(field.path);
        if (editor_it == config_editors_.end()) {
            continue;
        }
        yunlink::ConfigValue value;
        if (!field.choices.empty()) {
            const auto* combo = qobject_cast<QComboBox*>(editor_it->second);
            const int selected = combo == nullptr ? -1 : combo->currentIndex();
            if (selected < 0 || selected >= static_cast<int>(field.choices.size())) {
                continue;
            }
            value = field.choices[static_cast<std::size_t>(selected)].value;
        } else if (field.type == yunlink::ConfigValueType::kBool) {
            value = yunlink::ConfigValue::from_bool(
                qobject_cast<QCheckBox*>(editor_it->second)->isChecked());
        } else if (field.type == yunlink::ConfigValueType::kInt64) {
            value = int64_value_from_text(qobject_cast<QLineEdit*>(editor_it->second)->text());
        } else if (field.type == yunlink::ConfigValueType::kDouble) {
            value = double_value_from_text(qobject_cast<QLineEdit*>(editor_it->second)->text());
        } else if (field.type == yunlink::ConfigValueType::kStringList) {
            value = yunlink::ConfigValue::from_string_list(
                split_string_list(qobject_cast<QLineEdit*>(editor_it->second)->text()));
        } else {
            value = yunlink::ConfigValue::from_string(
                qobject_cast<QLineEdit*>(editor_it->second)->text().toStdString());
        }
        updates.push_back({field.path, std::move(value)});
    }
    return updates;
}
