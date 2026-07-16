#include "ui/main_window.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>

namespace {

const yunlink::ConfigFieldValue* find_snapshot_value(const yunlink::ConfigSnapshot& snapshot,
                                                     const std::string& path) {
    for (const auto& field : snapshot.values) {
        if (field.path == path) {
            return &field;
        }
    }
    return nullptr;
}

QString join_string_list(const std::vector<std::string>& values) {
    QStringList result;
    for (const auto& value : values) {
        result.append(QString::fromStdString(value));
    }
    return result.join(";");
}

QString join_double_list(const std::vector<double>& values) {
    QStringList result;
    for (const double value : values) {
        result.append(QString::number(value, 'g', 17));
    }
    return result.join(";");
}

QString status_text(const MonitorConfigurationState& state) {
    QString text =
        QString::fromStdString(state.last_status.empty() ? "等待配置响应" : state.last_status);
    if (!state.last_patch.errors.empty()) {
        QStringList errors;
        for (const auto& error : state.last_patch.errors) {
            errors.append(QString("%1: %2")
                              .arg(QString::fromStdString(error.path))
                              .arg(QString::fromStdString(error.message)));
        }
        text += "\n" + errors.join("\n");
    }
    return text;
}

bool same_schema(const std::vector<yunlink::ConfigFieldSchema>& lhs,
                 const std::vector<yunlink::ConfigFieldSchema>& rhs) {
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        const auto& left = lhs[index];
        const auto& right = rhs[index];
        if (left.path != right.path || left.title != right.title ||
            left.description != right.description || left.type != right.type ||
            left.read_only != right.read_only || left.sensitive != right.sensitive ||
            left.choices.size() != right.choices.size()) {
            return false;
        }
    }
    return true;
}

}  // namespace

void MainWindow::refresh_configuration() {
    if (backend_ == nullptr || config_resource_combo_ == nullptr ||
        config_fields_table_ == nullptr) {
        return;
    }
    const MonitorConfigurationState state = backend_->snapshot_configuration();
    config_status_label_->setText(status_text(state));
    if (!state.supported) {
        return;
    }
    if (state.resources.empty()) {
        if (!state.list_pending) {
            backend_->request_config_resource_list();
        }
        return;
    }

    const QString selected = config_resource_combo_->currentData().toString();
    {
        QSignalBlocker blocker(config_resource_combo_);
        config_resource_combo_->clear();
        for (const auto& resource : state.resources) {
            config_resource_combo_->addItem(QString::fromStdString(resource.title),
                                            QString::fromStdString(resource.id));
        }
        const int selected_index = config_resource_combo_->findData(selected);
        config_resource_combo_->setCurrentIndex(selected_index >= 0 ? selected_index : 0);
    }
    const std::string id = config_resource_combo_->currentData().toString().toStdString();
    const auto resource_it = state.resource_states.find(id);
    if (resource_it == state.resource_states.end()) {
        backend_->request_config_resource_describe(id);
        backend_->request_config_resource_get(id);
        return;
    }
    const auto& resource = resource_it->second;
    if (!resource.has_schema && !resource.schema_pending) {
        backend_->request_config_resource_describe(id);
    }
    if (!resource.has_snapshot && !resource.snapshot_pending) {
        backend_->request_config_resource_get(id);
    }
    if (!resource.has_schema || !resource.has_snapshot) {
        return;
    }

    const bool rebuild =
        config_editor_resource_id_ != id || !same_schema(config_editor_schema_, resource.fields);
    if (rebuild) {
        rebuild_configuration_editors(resource);
        config_editor_resource_id_ = id;
        config_loaded_revision_.clear();
    }
    if (config_loaded_revision_ != resource.snapshot.revision) {
        for (const auto& field : resource.fields) {
            const auto* current = find_snapshot_value(resource.snapshot, field.path);
            const auto editor_it = config_editors_.find(field.path);
            if (current == nullptr || editor_it == config_editors_.end()) {
                continue;
            }
            QWidget* editor = editor_it->second;
            if (!field.choices.empty()) {
                auto* combo = qobject_cast<QComboBox*>(editor);
                for (std::size_t index = 0; index < field.choices.size(); ++index) {
                    if (field.choices[index].value.type == current->value.type &&
                        field.choices[index].value.string_value == current->value.string_value &&
                        field.choices[index].value.int64_value == current->value.int64_value) {
                        combo->setCurrentIndex(static_cast<int>(index));
                        break;
                    }
                }
            } else if (field.type == yunlink::ConfigValueType::kBool) {
                qobject_cast<QCheckBox*>(editor)->setChecked(current->value.bool_value);
            } else if (field.type == yunlink::ConfigValueType::kInt64) {
                qobject_cast<QLineEdit*>(editor)->setText(
                    QString::number(static_cast<qlonglong>(current->value.int64_value)));
            } else if (field.type == yunlink::ConfigValueType::kDouble) {
                qobject_cast<QLineEdit*>(editor)->setText(
                    QString::number(current->value.double_value, 'g', 17));
            } else if (field.type == yunlink::ConfigValueType::kStringList) {
                qobject_cast<QLineEdit*>(editor)->setText(
                    join_string_list(current->value.string_list_value));
            } else if (field.type == yunlink::ConfigValueType::kDoubleList) {
                qobject_cast<QLineEdit*>(editor)->setText(
                    join_double_list(current->value.double_list_value));
            } else {
                qobject_cast<QLineEdit*>(editor)->setText(
                    QString::fromStdString(current->value.string_value));
            }
        }
        config_loaded_revision_ = resource.snapshot.revision;
    }
    config_validate_button_->setEnabled(resource.descriptor.writable);
    config_save_button_->setEnabled(resource.descriptor.writable);
    config_apply_button_->setEnabled(resource.descriptor.apply_supported &&
                                     resource.snapshot.revision !=
                                         resource.snapshot.applied_revision);
}
