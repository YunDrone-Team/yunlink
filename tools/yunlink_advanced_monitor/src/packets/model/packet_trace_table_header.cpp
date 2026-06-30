#include "packets/model/packet_trace_table_header.hpp"

#include <array>

#include <QHeaderView>
#include <QTableView>

void configure_packet_table_header(QTableView* table) {
    if (table == nullptr || table->horizontalHeader() == nullptr) {
        return;
    }
    auto* header = table->horizontalHeader();
    header->setStretchLastSection(false);
    header->setSectionResizeMode(QHeaderView::Interactive);

    constexpr std::array<int, 13> kColumnWidths{
        96, 48, 86, 116, 190, 106, 120, 88, 92, 96, 112, 76, 108};
    for (int column = 0; column < static_cast<int>(kColumnWidths.size()); ++column) {
        table->setColumnWidth(column, kColumnWidths[static_cast<size_t>(column)]);
    }
    header->setSectionResizeMode(4, QHeaderView::Stretch);
}
