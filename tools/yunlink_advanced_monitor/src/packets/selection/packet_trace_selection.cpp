#include "packets/selection/packet_trace_selection.hpp"

#include <algorithm>

#include <QItemSelectionModel>
#include <QScrollBar>
#include <QTableView>
#include <QTimer>

#include "packets/model/packet_trace_table_model.hpp"

void restore_table_horizontal_scroll(QTableView* table, int value) {
    if (table == nullptr || table->horizontalScrollBar() == nullptr) {
        return;
    }
    auto* bar = table->horizontalScrollBar();
    const int next_value = std::min(value, bar->maximum());
    bar->setValue(next_value);
    QTimer::singleShot(0, table, [table, next_value]() {
        if (table == nullptr || table->horizontalScrollBar() == nullptr) {
            return;
        }
        auto* delayed_bar = table->horizontalScrollBar();
        delayed_bar->setValue(std::min(next_value, delayed_bar->maximum()));
    });
}

bool update_packet_trace_selection(QTableView* table,
                                   PacketTraceTableModel* model,
                                   bool follow_latest,
                                   bool full_rebuild,
                                   uint64_t* selected_trace_id) {
    if (table == nullptr || model == nullptr || selected_trace_id == nullptr ||
        model->rowCount() <= 0) {
        return true;
    }
    if (follow_latest) {
        *selected_trace_id = model->last_trace_id();
    }
    int target_row = model->row_for_trace_id(*selected_trace_id);
    const bool selection_missing = target_row < 0;
    if (selection_missing) {
        target_row = model->rowCount() - 1;
        const auto* fallback = model->record_at(target_row);
        *selected_trace_id = fallback != nullptr ? fallback->trace_id : 0;
    }
    if (!follow_latest && !selection_missing && !full_rebuild) {
        return false;
    }

    const int horizontal_scroll =
        table->horizontalScrollBar() != nullptr ? table->horizontalScrollBar()->value() : 0;
    const QModelIndex index = model->index(target_row, 0);
    table->selectionModel()->setCurrentIndex(
        index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    if (follow_latest) {
        table->scrollTo(index);
        restore_table_horizontal_scroll(table, horizontal_scroll);
    }
    return true;
}
