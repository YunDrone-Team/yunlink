#ifndef YUNLINK_ADVANCED_MONITOR_UI_PACKET_TRACE_TABLE_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_UI_PACKET_TRACE_TABLE_MODEL_HPP

#include <cstdint>
#include <unordered_map>
#include <vector>

#include <QAbstractTableModel>

#include <yunlink/yunlink.hpp>

class PacketTraceTableModel : public QAbstractTableModel {
  public:
    explicit PacketTraceTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section,
                        Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    void set_records(std::vector<yunlink::PacketTraceRecord> records);
    void replace_records(std::vector<yunlink::PacketTraceRecord> records);
    void append_records(const std::vector<yunlink::PacketTraceRecord>& records);
    const yunlink::PacketTraceRecord* record_at(int row) const;
    const std::vector<yunlink::PacketTraceRecord>& records() const;
    uint64_t first_trace_id() const;
    uint64_t last_trace_id() const;
    bool contains_trace_id(uint64_t trace_id) const;
    int row_for_trace_id(uint64_t trace_id) const;

  private:
    std::vector<yunlink::PacketTraceRecord> records_;
    std::unordered_map<uint64_t, int> trace_id_to_row_;

    void rebuild_index();
};

#endif  // YUNLINK_ADVANCED_MONITOR_UI_PACKET_TRACE_TABLE_MODEL_HPP
