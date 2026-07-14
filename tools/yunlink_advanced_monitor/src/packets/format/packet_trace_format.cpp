#include "packets/format/packet_trace_format.hpp"

#include <sstream>

namespace {

std::string command_type_label(uint16_t type) {
    switch (static_cast<yunlink::CommandType>(type)) {
    case yunlink::CommandType::kTakeoff:
        return "TAKEOFF";
    case yunlink::CommandType::kLand:
        return "LAND";
    case yunlink::CommandType::kReturn:
        return "RETURN";
    case yunlink::CommandType::kGoto:
        return "MOVE_POINT";
    case yunlink::CommandType::kVelocitySetpoint:
        return "MOVE_VELOCITY";
    case yunlink::CommandType::kTrajectoryChunk:
        return "TRAJECTORY_CHUNK";
    case yunlink::CommandType::kFormationTask:
        return "FORMATION_TASK";
    }
    return std::to_string(type);
}

std::string state_snapshot_type_label(uint16_t type) {
    switch (static_cast<yunlink::StateSnapshotType>(type)) {
    case yunlink::StateSnapshotType::kVehicleCore:
        return "VehicleCore";
    case yunlink::StateSnapshotType::kPx4State:
        return "Px4State";
    case yunlink::StateSnapshotType::kOdomStatus:
        return "OdomStatus";
    case yunlink::StateSnapshotType::kUavControlFsmState:
        return "UavControlFsmState";
    case yunlink::StateSnapshotType::kUavControllerState:
        return "UavControllerState";
    case yunlink::StateSnapshotType::kGimbalParams:
        return "GimbalParams";
    case yunlink::StateSnapshotType::kLocalOdom:
        return "LocalOdom";
    case yunlink::StateSnapshotType::kUavControlCmd:
        return "UavControlCmd";
    case yunlink::StateSnapshotType::kUavControlState:
        return "UavControlState";
    case yunlink::StateSnapshotType::kOdomState:
        return "OdomState";
    case yunlink::StateSnapshotType::kSunrayRuntimeDiagnostic:
        return "SunrayRuntimeDiagnostic";
    case yunlink::StateSnapshotType::kCommandExecutionStatus:
        return "CommandExecutionStatus";
    }
    return std::to_string(type);
}

std::string system_service_type_label(uint16_t type) {
    switch (static_cast<yunlink::SystemServiceType>(type)) {
    case yunlink::SystemServiceType::kFeatureListRequest:
        return "FeatureListRequest";
    case yunlink::SystemServiceType::kFeatureListResponse:
        return "FeatureListResponse";
    case yunlink::SystemServiceType::kFeatureGetRequest:
        return "FeatureGetRequest";
    case yunlink::SystemServiceType::kFeatureGetResponse:
        return "FeatureGetResponse";
    case yunlink::SystemServiceType::kFeatureStartRequest:
        return "FeatureStartRequest";
    case yunlink::SystemServiceType::kFeatureStartResponse:
        return "FeatureStartResponse";
    case yunlink::SystemServiceType::kFeatureStopRequest:
        return "FeatureStopRequest";
    case yunlink::SystemServiceType::kFeatureStopResponse:
        return "FeatureStopResponse";
    }
    return std::to_string(type);
}

std::string configuration_service_type_label(uint16_t type) {
    switch (static_cast<yunlink::ConfigurationServiceType>(type)) {
    case yunlink::ConfigurationServiceType::kResourceListRequest:
        return "ConfigResourceListRequest";
    case yunlink::ConfigurationServiceType::kResourceListResponse:
        return "ConfigResourceListResponse";
    case yunlink::ConfigurationServiceType::kResourceDescribeRequest:
        return "ConfigResourceDescribeRequest";
    case yunlink::ConfigurationServiceType::kResourceDescribeResponse:
        return "ConfigResourceDescribeResponse";
    case yunlink::ConfigurationServiceType::kResourceGetRequest:
        return "ConfigResourceGetRequest";
    case yunlink::ConfigurationServiceType::kResourceGetResponse:
        return "ConfigResourceGetResponse";
    case yunlink::ConfigurationServiceType::kResourcePatchRequest:
        return "ConfigResourcePatchRequest";
    case yunlink::ConfigurationServiceType::kResourcePatchResponse:
        return "ConfigResourcePatchResponse";
    case yunlink::ConfigurationServiceType::kResourceApplyRequest:
        return "ConfigResourceApplyRequest";
    case yunlink::ConfigurationServiceType::kResourceApplyResponse:
        return "ConfigResourceApplyResponse";
    }
    return std::to_string(type);
}

}  // namespace

std::string packet_direction_label(yunlink::PacketTraceDirection direction) {
    return direction == yunlink::PacketTraceDirection::kTx ? "TX" : "RX";
}

std::string packet_stage_label(yunlink::PacketTraceStage stage) {
    switch (stage) {
    case yunlink::PacketTraceStage::kRawReceived:
        return "RawReceived";
    case yunlink::PacketTraceStage::kDecodeSucceeded:
        return "DecodeSucceeded";
    case yunlink::PacketTraceStage::kDecodeFailed:
        return "DecodeFailed";
    case yunlink::PacketTraceStage::kDispatchAccepted:
        return "DispatchAccepted";
    case yunlink::PacketTraceStage::kDispatchRejected:
        return "DispatchRejected";
    case yunlink::PacketTraceStage::kEncodedForSend:
        return "EncodedForSend";
    case yunlink::PacketTraceStage::kSendSucceeded:
        return "SendSucceeded";
    case yunlink::PacketTraceStage::kSendFailed:
        return "SendFailed";
    }
    return "Unknown";
}

std::string packet_transport_trace_label(yunlink::TransportType transport) {
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

std::string packet_family_label(yunlink::MessageFamily family) {
    switch (family) {
    case yunlink::MessageFamily::kSession:
        return "Session";
    case yunlink::MessageFamily::kAuthority:
        return "Authority";
    case yunlink::MessageFamily::kCommand:
        return "Command";
    case yunlink::MessageFamily::kCommandResult:
        return "CommandResult";
    case yunlink::MessageFamily::kStateSnapshot:
        return "StateSnapshot";
    case yunlink::MessageFamily::kStateEvent:
        return "StateEvent";
    case yunlink::MessageFamily::kBulkChannelDescriptor:
        return "BulkChannelDescriptor";
    case yunlink::MessageFamily::kSystemService:
        return "SystemService";
    case yunlink::MessageFamily::kConfigurationService:
        return "ConfigurationService";
    }
    return "Unknown";
}

std::string packet_message_type_label(yunlink::MessageFamily family, uint16_t message_type) {
    switch (family) {
    case yunlink::MessageFamily::kCommand:
        return command_type_label(message_type);
    case yunlink::MessageFamily::kCommandResult:
        return "CommandResult";
    case yunlink::MessageFamily::kStateSnapshot:
        return state_snapshot_type_label(message_type);
    case yunlink::MessageFamily::kSystemService:
        return system_service_type_label(message_type);
    case yunlink::MessageFamily::kConfigurationService:
        return configuration_service_type_label(message_type);
    case yunlink::MessageFamily::kSession:
    case yunlink::MessageFamily::kAuthority:
    case yunlink::MessageFamily::kStateEvent:
    case yunlink::MessageFamily::kBulkChannelDescriptor:
        return std::to_string(message_type);
    }
    return std::to_string(message_type);
}

std::string packet_qos_label(yunlink::QosClass qos) {
    switch (qos) {
    case yunlink::QosClass::kReliableOrdered:
        return "ReliableOrdered";
    case yunlink::QosClass::kReliableLatest:
        return "ReliableLatest";
    case yunlink::QosClass::kBestEffort:
        return "BestEffort";
    case yunlink::QosClass::kBulk:
        return "Bulk";
    }
    return "Unknown";
}

std::string packet_error_code_label(yunlink::ErrorCode code) {
    switch (code) {
    case yunlink::ErrorCode::kOk:
        return "OK";
    case yunlink::ErrorCode::kInvalidArgument:
        return "INVALID_ARGUMENT";
    case yunlink::ErrorCode::kSocketError:
        return "SOCKET_ERROR";
    case yunlink::ErrorCode::kBindError:
        return "BIND_ERROR";
    case yunlink::ErrorCode::kListenError:
        return "LISTEN_ERROR";
    case yunlink::ErrorCode::kConnectError:
        return "CONNECT_ERROR";
    case yunlink::ErrorCode::kTimeout:
        return "TIMEOUT";
    case yunlink::ErrorCode::kEncodeError:
        return "ENCODE_ERROR";
    case yunlink::ErrorCode::kDecodeError:
        return "DECODE_ERROR";
    case yunlink::ErrorCode::kChecksumMismatch:
        return "CHECKSUM_MISMATCH";
    case yunlink::ErrorCode::kInvalidHeader:
        return "INVALID_HEADER";
    case yunlink::ErrorCode::kRuntimeStopped:
        return "RUNTIME_STOPPED";
    case yunlink::ErrorCode::kProtocolMismatch:
        return "PROTOCOL_MISMATCH";
    case yunlink::ErrorCode::kUnauthorized:
        return "UNAUTHORIZED";
    case yunlink::ErrorCode::kRejected:
        return "REJECTED";
    case yunlink::ErrorCode::kInternal:
        return "INTERNAL";
    }
    return "UNKNOWN";
}

std::string packet_status_label(const yunlink::PacketTraceRecord& record) {
    if (record.code != yunlink::ErrorCode::kOk) {
        return packet_error_code_label(record.code);
    }
    if (record.stage == yunlink::PacketTraceStage::kDispatchRejected) {
        return "REJECTED";
    }
    if (record.stage == yunlink::PacketTraceStage::kDecodeFailed) {
        return "DECODE_FAILED";
    }
    if (record.stage == yunlink::PacketTraceStage::kSendFailed) {
        return "SEND_FAILED";
    }
    return "OK";
}

bool packet_trace_is_error(const yunlink::PacketTraceRecord& record) {
    return record.code != yunlink::ErrorCode::kOk ||
           record.stage == yunlink::PacketTraceStage::kDecodeFailed ||
           record.stage == yunlink::PacketTraceStage::kDispatchRejected ||
           record.stage == yunlink::PacketTraceStage::kSendFailed;
}
