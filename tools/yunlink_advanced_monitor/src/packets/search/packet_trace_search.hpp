#ifndef YUNLINK_ADVANCED_MONITOR_PACKETS_SEARCH_PACKET_TRACE_SEARCH_HPP
#define YUNLINK_ADVANCED_MONITOR_PACKETS_SEARCH_PACKET_TRACE_SEARCH_HPP

#include <QString>
#include <yunlink/yunlink.hpp>

bool packet_trace_matches_search(const yunlink::PacketTraceRecord& record,
                                 const QString& needle);

#endif  // YUNLINK_ADVANCED_MONITOR_PACKETS_SEARCH_PACKET_TRACE_SEARCH_HPP
