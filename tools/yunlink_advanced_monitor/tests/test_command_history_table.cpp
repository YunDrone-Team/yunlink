#include <QApplication>
#include <QHeaderView>
#include <QTableWidget>
#include <QTableWidgetItem>

#include "ui/commands/command_history_table.hpp"

int main(int argc, char** argv) {
    QApplication app(argc, argv);

    QTableWidget table;
    table.setColumnCount(7);
    table.setRowCount(1);
    monitor_ui::configure_command_history_table(&table);

    if (!table.wordWrap()) {
        return 1;
    }
    if (table.textElideMode() != Qt::ElideNone) {
        return 2;
    }
    if (table.verticalHeader()->sectionResizeMode(0) != QHeaderView::ResizeToContents) {
        return 3;
    }
    if (table.horizontalHeader()->sectionResizeMode(1) != QHeaderView::Stretch ||
        table.horizontalHeader()->sectionResizeMode(6) != QHeaderView::Stretch) {
        return 4;
    }

    table.setItem(0, 1, new QTableWidgetItem("MOVE_POINT\nx=1.35 y=1.08 z=0.70 yaw=0.0"));
    table.resize(1000, 300);
    table.show();
    app.processEvents();
    table.resizeRowsToContents();

    if (table.rowHeight(0) < table.fontMetrics().lineSpacing() * 2) {
        return 5;
    }
    return 0;
}
