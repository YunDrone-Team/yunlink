#ifndef YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_UI_STYLE_HPP
#define YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_UI_STYLE_HPP

#include <cstdint>
#include <string>

#include <QString>

#include "model/monitor_topics.hpp"
#include "model/monitor_state.hpp"

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QTableWidgetItem;
class QWidget;

namespace monitor_ui {

enum class Level {
    kNeutral,
    kInfo,
    kOk,
    kWarn,
    kError,
};

enum class ButtonRole {
    kPrimary,
    kSecondary,
    kDanger,
    kWarning,
};

void apply_window_style(QWidget* widget);
void style_button(QPushButton* button, ButtonRole role);
void style_table(QTableWidget* table);
void style_log_view(QPlainTextEdit* log_view);
void configure_copyable_log_view(QPlainTextEdit* log_view, int max_blocks = 0);
void set_tag(QLabel* label, Level level, const QString& text);
void set_mono(QLabel* label);
void style_item(QTableWidgetItem* item, bool mono = false);
void set_status_item(QTableWidgetItem* item, Level level);

Level level_from_status(const std::string& text);
Level level_from_log(const std::string& level);
QString tag_html(Level level, const QString& text);
QString inline_notice_html(Level level, const QString& title, const QString& detail);
QString topic_summary_text(const MonitorTopicState& topic, uint64_t now_ms);
QString command_context_text(const MonitorConnectionSnapshot& snapshot);
QString command_gate_notice(bool session_ready,
                            bool authority_ready,
                            bool exec_stale,
                            bool has_exec,
                            bool ready_takeoff,
                            bool ready_land,
                            const std::string& command_name,
                            const std::string& execution_name,
                            const std::string& reason_text);
QString feature_request_preview(const QString& feature,
                                const QString& args,
                                bool restart,
                                bool terminal,
                                bool force_stop);

bool confirm_risky_command(QWidget* parent,
                           const QString& command,
                           const QString& context,
                           const QString& payload);
bool confirm_warning(QWidget* parent, const QString& title, const QString& detail);

}  // namespace monitor_ui

#endif  // YUNLINK_ADVANCED_MONITOR_COMMON_MONITOR_UI_STYLE_HPP
