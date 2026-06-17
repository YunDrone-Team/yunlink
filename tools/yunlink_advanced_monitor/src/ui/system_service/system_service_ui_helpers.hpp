#ifndef YUNLINK_ADVANCED_MONITOR_UI_SYSTEM_SERVICE_UI_HELPERS_HPP
#define YUNLINK_ADVANCED_MONITOR_UI_SYSTEM_SERVICE_UI_HELPERS_HPP

#include <string>
#include <vector>

#include <QString>

class QPlainTextEdit;
class QListWidget;

namespace monitor_system_service_ui {

std::string bool_label(bool value);
std::string join_values(const std::vector<std::string>& values, const std::string& sep);
std::vector<std::string> split_override_args(const std::string& text);
void update_feature_list_if_changed(QListWidget* list,
                                    const std::string& status,
                                    const std::vector<std::string>& feature_names,
                                    const QString& selected_feature);
void update_plain_text_if_changed(QPlainTextEdit* editor, const QString& text);

}  // namespace monitor_system_service_ui

#endif  // YUNLINK_ADVANCED_MONITOR_UI_SYSTEM_SERVICE_UI_HELPERS_HPP
