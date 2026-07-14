#include "packets/model/packet_trace_table_model.hpp"

#include <QBrush>
#include <QColor>
#include <QDateTime>
#include <QFont>

#include "packets/format/packet_trace_format.hpp"

namespace {

enum Column {
    kTime = 0,
    kDirection,
    kStage,
    kTransport,
    kPeer,
    kFamily,
    kType,
    kQos,
    kSession,
    kMessage,
    kCorrelation,
    kPayloadLen,
    kStatus,
    kColumnCount,
};

QString time_text(uint64_t observed_at_ms) {
    return QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(observed_at_ms))
        .toString("HH:mm:ss.zzz");
}

QVariant mono_font() {
    QFont font;
    font.setFamily("Menlo");
    font.setStyleHint(QFont::Monospace);
    return font;
}

}  // namespace

PacketTraceTableModel::PacketTraceTableModel(QObject* parent) : QAbstractTableModel(parent) {}

int PacketTraceTableModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(records_.size());
}

int PacketTraceTableModel::columnCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : kColumnCount;
}

QVariant PacketTraceTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(records_.size())) {
        return {};
    }
    const auto& record = records_[static_cast<size_t>(index.row())];
    const auto& envelope = record.envelope;

    if (role == Qt::ForegroundRole && packet_trace_is_error(record)) {
        return QBrush(QColor("#da1e28"));
    }
    if (role == Qt::FontRole && index.column() >= kSession) {
        return mono_font();
    }
    if (role != Qt::DisplayRole) {
        return {};
    }

    switch (index.column()) {
    case kTime:
        return time_text(record.observed_at_ms);
    case kDirection:
        return QString::fromStdString(packet_direction_label(record.direction));
    case kStage:
        return QString::fromStdString(packet_stage_label(record.stage));
    case kTransport:
        return QString::fromStdString(packet_transport_trace_label(record.transport));
    case kPeer:
        return record.peer.id.empty() ? "-" : QString::fromStdString(record.peer.id);
    case kFamily:
        return record.has_envelope ? QString::fromStdString(packet_family_label(envelope.message_family))
                                   : "-";
    case kType:
        return record.has_envelope
                   ? QString::fromStdString(
                         packet_message_type_label(envelope.message_family, envelope.message_type))
                   : "-";
    case kQos:
        return record.has_envelope ? QString::fromStdString(packet_qos_label(envelope.qos_class))
                                   : "-";
    case kSession:
        return record.has_envelope ? QString::number(static_cast<qulonglong>(envelope.session_id))
                                   : "-";
    case kMessage:
        return record.has_envelope ? QString::number(static_cast<qulonglong>(envelope.message_id))
                                   : "-";
    case kCorrelation:
        return record.has_envelope
                   ? QString::number(static_cast<qulonglong>(envelope.correlation_id))
                   : "-";
    case kPayloadLen:
        return QString::number(record.payload_len);
    case kStatus:
        return QString::fromStdString(packet_status_label(record));
    default:
        break;
    }
    return {};
}

QVariant PacketTraceTableModel::headerData(int section,
                                           Qt::Orientation orientation,
                                           int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }
    switch (section) {
    case kTime:
        return "时间";
    case kDirection:
        return "方向";
    case kStage:
        return "阶段";
    case kTransport:
        return "transport";
    case kPeer:
        return "peer";
    case kFamily:
        return "family";
    case kType:
        return "type";
    case kQos:
        return "qos";
    case kSession:
        return "session";
    case kMessage:
        return "message";
    case kCorrelation:
        return "correlation";
    case kPayloadLen:
        return "payload";
    case kStatus:
        return "状态";
    default:
        return {};
    }
}

void PacketTraceTableModel::set_records(std::vector<yunlink::PacketTraceRecord> records) {
    replace_records(std::move(records));
}

void PacketTraceTableModel::replace_records(std::vector<yunlink::PacketTraceRecord> records) {
    beginResetModel();
    records_ = std::move(records);
    rebuild_index();
    endResetModel();
}

void PacketTraceTableModel::append_records(const std::vector<yunlink::PacketTraceRecord>& records) {
    if (records.empty()) {
        return;
    }
    const int first = static_cast<int>(records_.size());
    const int last = first + static_cast<int>(records.size()) - 1;
    beginInsertRows(QModelIndex(), first, last);
    records_.insert(records_.end(), records.begin(), records.end());
    for (int row = first; row <= last; ++row) {
        const auto trace_id = records_[static_cast<size_t>(row)].trace_id;
        if (trace_id != 0) {
            trace_id_to_row_[trace_id] = row;
        }
    }
    endInsertRows();
}

const yunlink::PacketTraceRecord* PacketTraceTableModel::record_at(int row) const {
    if (row < 0 || row >= static_cast<int>(records_.size())) {
        return nullptr;
    }
    return &records_[static_cast<size_t>(row)];
}

const std::vector<yunlink::PacketTraceRecord>& PacketTraceTableModel::records() const {
    return records_;
}

uint64_t PacketTraceTableModel::first_trace_id() const {
    return records_.empty() ? 0 : records_.front().trace_id;
}

uint64_t PacketTraceTableModel::last_trace_id() const {
    return records_.empty() ? 0 : records_.back().trace_id;
}

bool PacketTraceTableModel::contains_trace_id(uint64_t trace_id) const {
    return trace_id_to_row_.find(trace_id) != trace_id_to_row_.end();
}

int PacketTraceTableModel::row_for_trace_id(uint64_t trace_id) const {
    const auto it = trace_id_to_row_.find(trace_id);
    return it == trace_id_to_row_.end() ? -1 : it->second;
}

void PacketTraceTableModel::rebuild_index() {
    trace_id_to_row_.clear();
    for (int row = 0; row < static_cast<int>(records_.size()); ++row) {
        const auto trace_id = records_[static_cast<size_t>(row)].trace_id;
        if (trace_id != 0) {
            trace_id_to_row_[trace_id] = row;
        }
    }
}
