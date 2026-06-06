#include "model/monitor_state.hpp"

std::string level_label(MonitorLogLevel level) {
    switch (level) {
    case MonitorLogLevel::kInfo:
        return "INFO";
    case MonitorLogLevel::kWarn:
        return "WARN";
    case MonitorLogLevel::kError:
        return "ERROR";
    }
    return "INFO";
}

std::string source_label(MonitorLogSource source) {
    switch (source) {
    case MonitorLogSource::kRuntime:
        return "Runtime";
    case MonitorLogSource::kSession:
        return "Session";
    case MonitorLogSource::kSystemService:
        return "SystemService";
    }
    return "Runtime";
}

std::string transport_label(yunlink::TransportType transport) {
    switch (transport) {
    case yunlink::TransportType::kTcpServer:
        return "TCP_SERVER";
    case yunlink::TransportType::kTcpClient:
        return "TCP_CLIENT";
    case yunlink::TransportType::kUdpUnicast:
        return "UDP_UNICAST";
    case yunlink::TransportType::kUdpBroadcast:
        return "UDP_BROADCAST";
    case yunlink::TransportType::kUdpMulticast:
        return "UDP_MULTICAST";
    }
    return "UNKNOWN";
}

std::string command_lifecycle_label(MonitorCommandLifecycle lifecycle) {
    switch (lifecycle) {
    case MonitorCommandLifecycle::kSent:
        return "SENT";
    case MonitorCommandLifecycle::kReceived:
        return "RECEIVED";
    case MonitorCommandLifecycle::kTimeout:
        return "TIMEOUT";
    }
    return "SENT";
}
