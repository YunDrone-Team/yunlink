#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_DETAIL_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_DETAIL_HPP

#include <string>

#include <yunlink/yunlink.hpp>

std::string packet_summary_detail(const yunlink::PacketTraceRecord& record);
std::string packet_header_detail(const yunlink::PacketTraceRecord& record);
std::string packet_source_target_detail(const yunlink::PacketTraceRecord& record);
std::string packet_security_detail(const yunlink::PacketTraceRecord& record);
std::string packet_payload_detail(const yunlink::PacketTraceRecord& record);
std::string packet_raw_detail(const yunlink::PacketTraceRecord& record);
std::string packet_errors_detail(const yunlink::PacketTraceRecord& record);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_PACKET_TRACE_DETAIL_HPP
