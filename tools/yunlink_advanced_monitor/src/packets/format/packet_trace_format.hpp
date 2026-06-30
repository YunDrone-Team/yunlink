#ifndef YUNLINK_ADVANCED_MONITOR_COMMON_PACKET_TRACE_FORMAT_HPP
#define YUNLINK_ADVANCED_MONITOR_COMMON_PACKET_TRACE_FORMAT_HPP

#include <string>

#include <yunlink/yunlink.hpp>

std::string packet_direction_label(yunlink::PacketTraceDirection direction);
std::string packet_stage_label(yunlink::PacketTraceStage stage);
std::string packet_transport_trace_label(yunlink::TransportType transport);
std::string packet_family_label(yunlink::MessageFamily family);
std::string packet_message_type_label(yunlink::MessageFamily family, uint16_t message_type);
std::string packet_qos_label(yunlink::QosClass qos);
std::string packet_error_code_label(yunlink::ErrorCode code);
std::string packet_status_label(const yunlink::PacketTraceRecord& record);
bool packet_trace_is_error(const yunlink::PacketTraceRecord& record);
std::string packet_endpoint_label(const yunlink::EndpointIdentity& identity);
std::string packet_target_label(const yunlink::TargetSelector& target);
std::string packet_hex_dump(const yunlink::ByteBuffer& bytes);
std::string packet_ascii_preview(const yunlink::ByteBuffer& bytes);

#endif  // YUNLINK_ADVANCED_MONITOR_COMMON_PACKET_TRACE_FORMAT_HPP
