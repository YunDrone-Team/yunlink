#include "common/monitor_ui_style.hpp"

#include <QAbstractButton>
#include <QMessageBox>

namespace monitor_ui {

bool confirm_risky_command(QWidget* parent,
                           const QString& command,
                           const QString& context,
                           const QString& payload) {
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(command + " confirmation");
    box.setText(command + " 是高风险控制指令。");
    box.setInformativeText(context + "\n\nPayload\n" + payload +
                           "\n\n确认后将通过 YunLink command plane 发送，并等待 CommandResult / execution status。");
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Confirm send");
    box.button(QMessageBox::Cancel)->setText("Cancel");
    return box.exec() == QMessageBox::Ok;
}

bool confirm_warning(QWidget* parent, const QString& title, const QString& detail) {
    QMessageBox box(parent);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(title);
    box.setText(title);
    box.setInformativeText(detail);
    box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
    box.button(QMessageBox::Ok)->setText("Confirm");
    box.button(QMessageBox::Cancel)->setText("Cancel");
    return box.exec() == QMessageBox::Ok;
}

}  // namespace monitor_ui
