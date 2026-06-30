#include "packets/packet_trace_detail.hpp"

#include <sstream>

#include "packets/format/packet_trace_format.hpp"

namespace {

std::string bool_text(bool value) {
    return value ? "true" : "false";
}

std::string envelope_summary(const yunlink::PacketTraceRecord& record) {
    if (!record.has_envelope) {
        return "No decoded SecureEnvelope for this trace stage.";
    }
    const auto& e = record.envelope;
    std::ostringstream ss;
    ss << "family: " << packet_family_label(e.message_family) << "\n";
    ss << "message_type: " << packet_message_type_label(e.message_family, e.message_type)
       << " (" << e.message_type << ")\n";
    ss << "qos: " << packet_qos_label(e.qos_class) << "\n";
    ss << "session_id: " << e.session_id << "\n";
    ss << "message_id: " << e.message_id << "\n";
    ss << "correlation_id: " << e.correlation_id << "\n";
    ss << "payload_len: " << e.payload_len << "\n";
    ss << "checksum: " << e.checksum << "\n";
    return ss.str();
}

}  // namespace

std::string packet_summary_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream summary;
    summary << "trace_id: " << record.trace_id << "\n";
    summary << "direction: " << packet_direction_label(record.direction) << "\n";
    summary << "stage: " << packet_stage_label(record.stage) << "\n";
    summary << "transport: " << packet_transport_trace_label(record.transport) << "\n";
    summary << "peer: " << (record.peer.id.empty() ? "-" : record.peer.id) << "\n";
    summary << "status: " << packet_status_label(record) << "\n\n";
    summary << envelope_summary(record);
    return summary.str();
}

std::string packet_header_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << "magic: SURY\n";
    ss << "header_len: " << record.header_len << "\n";
    ss << "payload_len: " << record.payload_len << "\n";
    ss << "total_len: " << record.total_len << "\n";
    ss << "checksum: " << record.checksum << "\n";
    if (!record.has_envelope) {
        ss << "decoded: false\n";
        return ss.str();
    }
    const auto& e = record.envelope;
    ss << "decoded: true\n";
    ss << "protocol_major: " << static_cast<int>(e.protocol_major) << "\n";
    ss << "header_version: " << static_cast<int>(e.header_version) << "\n";
    ss << "flags: " << e.flags << "\n";
    ss << "message_family: " << packet_family_label(e.message_family) << " ("
       << static_cast<int>(e.message_family) << ")\n";
    ss << "message_type: " << packet_message_type_label(e.message_family, e.message_type) << " ("
       << e.message_type << ")\n";
    ss << "schema_version: " << e.schema_version << "\n";
    ss << "created_at_ms: " << e.created_at_ms << "\n";
    ss << "ttl_ms: " << e.ttl_ms << "\n";
    return ss.str();
}

std::string packet_source_target_detail(const yunlink::PacketTraceRecord& record) {
    if (!record.has_envelope) {
        return "No decoded source/target fields.";
    }
    std::ostringstream ss;
    ss << "peer: " << (record.peer.id.empty() ? "-" : record.peer.id) << "\n";
    ss << "source: " << packet_endpoint_label(record.envelope.source) << "\n";
    ss << "target: " << packet_target_label(record.envelope.target) << "\n";
    if (record.metadata_truncated) {
        ss << "metadata_truncated: true\n";
        ss << "target_ids_total: " << record.target_ids_total << "\n";
    }
    return ss.str();
}

std::string packet_security_detail(const yunlink::PacketTraceRecord& record) {
    if (!record.has_envelope) {
        return "No decoded security/QoS fields.";
    }
    const auto& e = record.envelope;
    std::ostringstream ss;
    ss << "qos_class: " << packet_qos_label(e.qos_class) << "\n";
    ss << "key_epoch: " << e.security.key_epoch << "\n";
    ss << "auth_tag_len: " << record.auth_tag_total << "\n";
    ss << "auth_tag_preview_len: " << e.security.auth_tag.size() << "\n";
    ss << "ttl_ms: " << e.ttl_ms << "\n";
    ss << "flags: " << e.flags << "\n";
    ss << "metadata_truncated: " << bool_text(record.metadata_truncated) << "\n";
    return ss.str();
}

std::string packet_payload_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << "payload_len: " << record.payload_len << "\n";
    ss << "preview_len: " << record.payload_preview.size() << "\n";
    ss << "truncated: " << bool_text(record.payload_truncated) << "\n\n";
    ss << "ASCII preview:\n" << packet_ascii_preview(record.payload_preview) << "\n\n";
    ss << "Hex preview:\n" << packet_hex_dump(record.payload_preview);
    return ss.str();
}

std::string packet_raw_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << "total_len: " << record.total_len << "\n";
    ss << "raw_preview_len: " << record.raw_preview.size() << "\n";
    ss << "truncated: " << bool_text(record.raw_truncated) << "\n\n";
    ss << packet_hex_dump(record.raw_preview);
    return ss.str();
}

std::string packet_errors_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    ss << "status: " << packet_status_label(record) << "\n";
    ss << "code: " << packet_error_code_label(record.code) << "\n";
    ss << "stage: " << packet_stage_label(record.stage) << "\n";
    ss << "message: " << (record.error_message.empty() ? "-" : record.error_message) << "\n";
    return ss.str();
}
