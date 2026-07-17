#include "common/monitor_ui_style.hpp"
#include <algorithm>

#include <QBrush>
#include <QColor>
#include <QFont>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QWidget>

#include "common/monitor_format.hpp"

namespace monitor_ui {
namespace {

struct Palette {
    const char* bg = "#f4f4f4";
    const char* layer = "#ffffff";
    const char* layer_alt = "#f4f4f4";
    const char* border = "#c6c6c6";
    const char* text = "#161616";
    const char* muted = "#525252";
    const char* focus = "#0f62fe";
    const char* ok = "#24a148";
    const char* warn = "#8a6d00";
    const char* warn_bg = "#fcf4d6";
    const char* error = "#da1e28";
    const char* error_bg = "#fff1f1";
    const char* info_bg = "#edf5ff";
};

const Palette& palette() {
    static const Palette p;
    return p;
}

QString mono_family() { return "'IBM Plex Mono','SF Mono','Menlo','DejaVu Sans Mono',monospace"; }

QString color_for(Level level) {
    const auto& p = palette();
    switch (level) {
    case Level::kOk: return p.ok;
    case Level::kWarn: return p.warn;
    case Level::kError: return p.error;
    case Level::kInfo: return p.focus;
    case Level::kNeutral: return p.muted;
    }
    return p.muted;
}

QString background_for(Level level) {
    const auto& p = palette();
    switch (level) {
    case Level::kOk: return "#defbe6";
    case Level::kWarn: return p.warn_bg;
    case Level::kError: return p.error_bg;
    case Level::kInfo: return p.info_bg;
    case Level::kNeutral: return p.layer_alt;
    }
    return p.layer_alt;
}

bool contains_any(std::string text, std::initializer_list<const char*> needles) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });
    for (const char* needle : needles) {
        if (text.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // namespace

void apply_window_style(QWidget* widget) {
    if (widget == nullptr) {
        return;
    }
    const auto& p = palette();
    const QString css =
        "QMainWindow,QWidget{background:%1;color:%2;font-family:'IBM Plex Sans','Inter','Arial';font-size:12px;}"
        "QGroupBox{background:%3;border:1px solid %4;border-radius:4px;margin-top:16px;padding:10px;font-weight:600;}"
        "QGroupBox::title{subcontrol-origin:margin;left:10px;padding:0 4px;color:%2;}QLabel{color:%2;}"
        "QLineEdit,QDoubleSpinBox,QComboBox{background:%3;border:1px solid %4;border-radius:2px;padding:4px 6px;min-height:24px;}"
        "QLineEdit:focus,QDoubleSpinBox:focus,QComboBox:focus{border:1px solid %5;}"
        "QTabWidget::pane{border:1px solid %4;background:%3;}QTabBar::tab{background:%6;border:1px solid %4;padding:7px 12px;margin-right:2px;}"
        "QTabBar::tab:selected{background:%3;border-bottom-color:%3;font-weight:600;}";
    widget->setStyleSheet(QString(css).arg(p.bg, p.text, p.layer, p.border, p.focus, p.layer_alt));
}

void style_button(QPushButton* button, ButtonRole role) {
    if (button == nullptr) {
        return;
    }
    const auto& p = palette();
    QString bg = p.layer;
    QString fg = p.text;
    QString border = p.border;
    if (role == ButtonRole::kPrimary) {
        bg = p.focus;
        fg = "#ffffff";
        border = p.focus;
    } else if (role == ButtonRole::kDanger) {
        bg = p.error;
        fg = "#ffffff";
        border = p.error;
    } else if (role == ButtonRole::kWarning) {
        bg = p.warn_bg;
        fg = "#5e4700";
        border = "#f1c21b";
    }
    button->setMinimumHeight(30);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QString("QPushButton { background:%1; color:%2; border:1px solid %3;"
                                  " border-radius:2px; padding:5px 12px; font-weight:600; }"
                                  "QPushButton:disabled { background:#e0e0e0; color:#8d8d8d;"
                                  " border-color:#c6c6c6; }")
                              .arg(bg, fg, border));
}

void style_table(QTableWidget* table) {
    if (table == nullptr) {
        return;
    }
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setDefaultSectionSize(28);
    table->setStyleSheet(QString("QTableWidget { background:#ffffff; alternate-background-color:#f4f4f4;"
                                 " gridline-color:#e0e0e0; selection-background-color:#d0e2ff;"
                                 " selection-color:#161616; }"
                                 "QHeaderView::section { background:#e0e0e0; color:#161616;"
                                 " border:0; border-right:1px solid #c6c6c6; padding:6px;"
                                 " font-weight:600; }"));
}

void style_log_view(QPlainTextEdit* log_view) {
    if (log_view == nullptr) {
        return;
    }
    log_view->setStyleSheet(QString("QPlainTextEdit { background:#262626; color:#f4f4f4;"
                                    " border:1px solid #525252; border-radius:4px; padding:8px;"
                                    " font-family:%1; font-size:12px; }")
                                .arg(mono_family()));
}

void set_tag(QLabel* label, Level level, const QString& text) {
    if (label == nullptr) {
        return;
    }
    label->setTextFormat(Qt::RichText);
    label->setText(tag_html(level, text));
}

void set_mono(QLabel* label) {
    if (label == nullptr) {
        return;
    }
    QFont font = label->font();
    font.setFamily("Menlo");
    font.setStyleHint(QFont::Monospace);
    label->setFont(font);
}

void style_item(QTableWidgetItem* item, bool mono) {
    if (item == nullptr) {
        return;
    }
    if (mono) {
        QFont font = item->font();
        font.setFamily("Menlo");
        font.setStyleHint(QFont::Monospace);
        item->setFont(font);
    }
}

void set_status_item(QTableWidgetItem* item, Level level) {
    if (item == nullptr) {
        return;
    }
    item->setForeground(QBrush(QColor(color_for(level))));
    item->setBackground(QBrush(QColor(background_for(level))));
    QFont font = item->font();
    font.setBold(true);
    item->setFont(font);
}

Level level_from_status(const std::string& text) {
    if (contains_any(text, {"ERROR", "FAILED", "INVALID", "LOST", "DOWN", "REJECTED", "TIMEOUT"})) {
        return Level::kError;
    }
    if (contains_any(text, {"WARN", "WAIT", "PENDING", "STALE", "RETRY", "RECONNECT", "SENT", "ACTIVE"})) {
        return Level::kWarn;
    }
    if (contains_any(text, {"OK", "UP", "RUNNING", "CONTROLLER", "SUCCEEDED", "CONNECTED", "READY"})) {
        return Level::kOk;
    }
    return Level::kNeutral;
}

Level level_from_log(const std::string& level) {
    return level_from_status(level);
}

QString tag_html(Level level, const QString& text) {
    return QString("<span style=\"display:inline-block;background:%1;color:%2;border:1px solid %2;"
                   "border-radius:2px;padding:2px 6px;font-family:%3;font-weight:600;\">%4</span>")
        .arg(background_for(level), color_for(level), mono_family(), text.toHtmlEscaped());
}

QString inline_notice_html(Level level, const QString& title, const QString& detail) {
    QString escaped_detail = detail.toHtmlEscaped();
    escaped_detail.replace("\n", "<br>");
    return QString("<div style=\"background:%1;border-left:4px solid %2;padding:8px;\">"
                   "<b>%3</b><br><span style=\"color:#525252;\">%4</span></div>")
        .arg(background_for(level), color_for(level), title.toHtmlEscaped(), escaped_detail);
}

QString topic_summary_text(const MonitorTopicState& topic, uint64_t now_ms) {
    if (!monitor_has_snapshot(topic.latest)) {
        return "状态: WAIT | 最近更新: -- | session: -- | message: --";
    }
    const uint64_t age_ms =
        now_ms >= topic.latest.received_at_ms ? now_ms - topic.latest.received_at_ms : 0;
    return QString("状态: ACTIVE | 最近更新: %1 | session: %2 | message: %3")
        .arg(QString::fromStdString(monitor_fmt_age_ms(age_ms)))
        .arg(topic.latest.session_id == 0 ? "--" : QString::number(topic.latest.session_id))
        .arg(topic.latest.message_id == 0 ? "--" : QString::number(topic.latest.message_id));
}

QString command_context_text(const MonitorConnectionSnapshot& snapshot) {
    return QString("目标=%1\npeer id=%2\nsession id=%3\n控制权=%4\n对端=%5")
        .arg(QString::fromStdString(snapshot.agent_label))
        .arg(snapshot.peer_id.empty() ? "-" : QString::fromStdString(snapshot.peer_id))
        .arg(snapshot.session_id == 0 ? "-" : QString::number(static_cast<qulonglong>(snapshot.session_id)))
        .arg(snapshot.authority_state.empty() ? "-" : QString::fromStdString(snapshot.authority_state))
        .arg(QString::fromStdString(snapshot.remote_endpoint));
}

QString command_gate_notice(bool session_ready,
                            bool authority_ready,
                            bool exec_stale,
                            bool has_exec,
                            bool ready_takeoff,
                            bool ready_land,
                            const std::string& command_name,
                            const std::string& execution_name,
                            const std::string& reason_text) {
    if (!session_ready) {
        return inline_notice_html(Level::kWarn,
                                  "缺少有效会话 (active session)",
                                  "当前尚未建立有效会话，命令已禁用。");
    }
    if (!authority_ready) {
        return inline_notice_html(Level::kWarn,
                                  "缺少有效控制权 (active authority)",
                                  "TAKEOFF / LAND / RETURN 需要有效控制权。MOVE_POINT / MOVE_VELOCITY 可发送，但是否接纳以 CommandResult 为准。");
    }
    if (exec_stale) {
        return inline_notice_html(Level::kWarn,
                                  "执行状态已过期",
                                  "command_execution_status 已过期，旧 ready/busy 门禁已忽略；请结合最新控制侧状态继续判断。");
    }
    if (has_exec && !ready_takeoff && !ready_land && reason_text != "-") {
        return inline_notice_html(Level::kWarn,
                                  "载具门禁未就绪",
                                  QString::fromStdString(reason_text + "。LAND/TAKEOFF 按 ready/busy 字段门禁。"));
    }
    if (has_exec) {
        std::string detail = "当前命令=" + command_name + " exec=" + execution_name +
                             " 可起飞=" + (ready_takeoff ? std::string("yes") : std::string("no")) +
                             " 可降落=" + (ready_land ? std::string("yes") : std::string("no"));
        if (reason_text != "-") {
            detail += " 原因=" + reason_text;
        }
        return inline_notice_html(Level::kOk, "命令门禁已就绪", QString::fromStdString(detail));
    }
    return inline_notice_html(Level::kOk,
                              "命令门禁已就绪",
                              "当前会话与控制权已就绪。高风险指令会先确认，再下发 YunLink command。");
}

QString feature_request_preview(const QString& feature,
                                const QString& args,
                                bool restart,
                                bool terminal,
                                bool force_stop) {
    return inline_notice_html(force_stop ? Level::kWarn : Level::kInfo,
                              "系统服务请求",
                              QString("feature=%1 | override_args=%2 | restart=%3 | terminal=%4 | force_stop=%5")
                                  .arg(feature.isEmpty() ? "-" : feature)
                                  .arg(args.isEmpty() ? "-" : args)
                                  .arg(restart ? "true" : "false")
                                  .arg(terminal ? "true" : "false")
                                  .arg(force_stop ? "true" : "false"));
}

}  // namespace monitor_ui
