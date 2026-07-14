#include "ui/system_service/system_service_ui_helpers.hpp"

#include <algorithm>

#include <QListWidget>
#include <QListWidgetItem>
#include <QPlainTextEdit>
#include <QScrollBar>
#include <QTextCursor>

namespace monitor_system_service_ui {
namespace {

bool feature_list_matches(QListWidget* list,
                          const std::string& status,
                          const std::vector<std::string>& feature_names) {
    int row = 0;
    if (!status.empty()) {
        if (list->count() <= row ||
            list->item(row)->text() != QString::fromStdString("状态: " + status) ||
            !list->item(row)->data(Qt::UserRole).toString().isEmpty()) {
            return false;
        }
        ++row;
    }
    if (list->count() != row + static_cast<int>(feature_names.size())) {
        return false;
    }
    for (const auto& name : feature_names) {
        const QString feature_name = QString::fromStdString(name);
        if (list->item(row)->text() != feature_name ||
            list->item(row)->data(Qt::UserRole).toString() != feature_name) {
            return false;
        }
        ++row;
    }
    return true;
}

}  // namespace

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

void update_feature_list_if_changed(QListWidget* list,
                                    const std::string& status,
                                    const std::vector<std::string>& feature_names,
                                    const QString& selected_feature) {
    if (list == nullptr || feature_list_matches(list, status, feature_names)) {
        return;
    }

    QScrollBar* scroll_bar = list->verticalScrollBar();
    const int old_scroll_value = scroll_bar == nullptr ? 0 : scroll_bar->value();
    list->clear();
    if (!status.empty()) {
        auto* status_item = new QListWidgetItem(QString::fromStdString("状态: " + status));
        status_item->setFlags(status_item->flags() & ~Qt::ItemIsSelectable);
        list->addItem(status_item);
    }
    for (const auto& name : feature_names) {
        const QString feature_name = QString::fromStdString(name);
        auto* item = new QListWidgetItem(feature_name);
        item->setData(Qt::UserRole, feature_name);
        list->addItem(item);
        if (feature_name == selected_feature) {
            list->setCurrentItem(item);
        }
    }
    if (scroll_bar != nullptr) {
        scroll_bar->setValue(std::min(old_scroll_value, scroll_bar->maximum()));
    }
}

void update_plain_text_if_changed(QPlainTextEdit* editor, const QString& text) {
    if (editor == nullptr || editor->textCursor().hasSelection() ||
        editor->toPlainText() == text) {
        return;
    }

    QScrollBar* scroll_bar = editor->verticalScrollBar();
    const int old_scroll_value = scroll_bar == nullptr ? 0 : scroll_bar->value();
    editor->setPlainText(text);
    if (scroll_bar != nullptr) {
        scroll_bar->setValue(std::min(old_scroll_value, scroll_bar->maximum()));
    }
}

}  // namespace monitor_system_service_ui
