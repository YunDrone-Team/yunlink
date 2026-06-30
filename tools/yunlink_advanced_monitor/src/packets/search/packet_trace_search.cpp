#include "packets/search/packet_trace_search.hpp"

#include <sstream>

#include "packets/format/packet_trace_format.hpp"

namespace {

std::string packet_search_text(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << record.trace_id << ' ' << record.observed_at_ms << ' '
       << packet_direction_label(record.direction) << ' ' << packet_stage_label(record.stage)
       << ' ' << packet_transport_trace_label(record.transport) << ' '
       << packet_status_label(record) << ' ' << record.payload_len << ' ' << record.total_len
       << ' ' << record.peer.id << ' ' << record.peer.ip << ' ' << record.error_message;
    if (record.has_envelope) {
        ss << ' ' << packet_family_label(record.envelope.message_family) << ' '
           << packet_message_type_label(record.envelope.message_family,
                                        record.envelope.message_type)
           << ' ' << packet_qos_label(record.envelope.qos_class) << ' '
           << record.envelope.session_id << ' ' << record.envelope.message_id << ' '
           << record.envelope.correlation_id << ' '
           << packet_endpoint_label(record.envelope.source) << ' '
           << packet_target_label(record.envelope.target);
    }
    return ss.str();
}

}  // namespace

bool packet_trace_matches_search(const yunlink::PacketTraceRecord& record,
                                 const QString& needle) {
    return QString::fromStdString(packet_search_text(record))
        .contains(needle, Qt::CaseInsensitive);
}
