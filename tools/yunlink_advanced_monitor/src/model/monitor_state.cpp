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
    case MonitorLogSource::kConnection:
        return "Connection";
    case MonitorLogSource::kAuthority:
        return "Authority";
    case MonitorLogSource::kCommand:
        return "Command";
    case MonitorLogSource::kBridge:
        return "Bridge";
    case MonitorLogSource::kSystemService:
        return "System";
    case MonitorLogSource::kDebug:
        return "Debug";
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
    case MonitorCommandLifecycle::kActive:
        return "ACTIVE";
    case MonitorCommandLifecycle::kSucceeded:
        return "SUCCEEDED";
    case MonitorCommandLifecycle::kFailed:
        return "FAILED";
    case MonitorCommandLifecycle::kCancelled:
        return "CANCELLED";
    case MonitorCommandLifecycle::kTimeout:
        return "TIMEOUT";
    }
    return "SENT";
}

bool command_lifecycle_is_terminal(MonitorCommandLifecycle lifecycle) {
    return lifecycle == MonitorCommandLifecycle::kSucceeded ||
           lifecycle == MonitorCommandLifecycle::kFailed ||
           lifecycle == MonitorCommandLifecycle::kCancelled ||
           lifecycle == MonitorCommandLifecycle::kTimeout;
}

MonitorCommandLifecycle command_lifecycle_from_phase(yunlink::CommandPhase phase) {
    switch (phase) {
    case yunlink::CommandPhase::kReceived:
    case yunlink::CommandPhase::kAccepted:
    case yunlink::CommandPhase::kInProgress:
        return MonitorCommandLifecycle::kActive;
    case yunlink::CommandPhase::kSucceeded:
        return MonitorCommandLifecycle::kSucceeded;
    case yunlink::CommandPhase::kFailed:
        return MonitorCommandLifecycle::kFailed;
    case yunlink::CommandPhase::kCancelled:
        return MonitorCommandLifecycle::kCancelled;
    case yunlink::CommandPhase::kExpired:
        return MonitorCommandLifecycle::kTimeout;
    }
    return MonitorCommandLifecycle::kActive;
}
