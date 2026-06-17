#include "common/monitor_ui_style.hpp"

#include <QPlainTextEdit>

namespace monitor_ui {

void configure_copyable_log_view(QPlainTextEdit* log_view, int max_blocks) {
    if (log_view == nullptr) {
        return;
    }
    log_view->setReadOnly(true);
    log_view->setLineWrapMode(QPlainTextEdit::NoWrap);
    log_view->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    if (max_blocks > 0) {
        log_view->setMaximumBlockCount(max_blocks);
    }
    style_log_view(log_view);
}

}  // namespace monitor_ui
