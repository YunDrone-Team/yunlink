#ifndef YUNLINK_ADVANCED_MONITOR_CONFIGURATION_TRACE_FORMAT_HPP
#define YUNLINK_ADVANCED_MONITOR_CONFIGURATION_TRACE_FORMAT_HPP

#include <sstream>

#include <yunlink/diagnostics/packet_trace.hpp>

void append_configuration_trace(const yunlink::PacketTraceRecord& record,
                                std::ostringstream* output);

#endif  // YUNLINK_ADVANCED_MONITOR_CONFIGURATION_TRACE_FORMAT_HPP
