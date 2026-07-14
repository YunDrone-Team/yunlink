#include "ui/commands/command_history_table.hpp"

#include <QAbstractItemView>
#include <QHeaderView>
#include <QTableWidget>

namespace monitor_ui {

void configure_command_history_table(QTableWidget* table) {
    if (table == nullptr) {
        return;
    }

    auto* horizontal = table->horizontalHeader();
    horizontal->setStretchLastSection(false);
    horizontal->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontal->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    horizontal->setSectionResizeMode(5, QHeaderView::Stretch);
    horizontal->setSectionResizeMode(6, QHeaderView::Stretch);

    table->setWordWrap(true);
    table->setTextElideMode(Qt::ElideNone);
    table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
}

}  // namespace monitor_ui
