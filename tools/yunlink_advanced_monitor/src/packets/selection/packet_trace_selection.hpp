#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_SELECTION_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_SELECTION_HPP

#include <cstdint>

class QTableView;
class PacketTraceTableModel;

bool update_packet_trace_selection(QTableView* table,
                                   PacketTraceTableModel* model,
                                   bool follow_latest,
                                   bool full_rebuild,
                                   uint64_t* selected_trace_id);
void restore_table_horizontal_scroll(QTableView* table, int value);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_SELECTION_HPP
