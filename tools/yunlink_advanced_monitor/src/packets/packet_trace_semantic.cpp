#include "packets/packet_trace_semantic.hpp"

#include <sstream>

#include "packets/format/packet_trace_format.hpp"
#include "packets/format/configuration_trace_format.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace {

std::string yes_no(bool value) {
    return value ? "true" : "false";
}

bool payload_available(const yunlink::PacketTraceRecord& record, std::ostringstream* ss) {
    if (record.payload_truncated ||
        record.payload_preview.size() != static_cast<size_t>(record.payload_len)) {
        *ss << "semantic decode skipped: payload preview is truncated or incomplete.\n";
        *ss << "payload_len: " << record.payload_len << "\n";
        *ss << "preview_len: " << record.payload_preview.size() << "\n";
        *ss << "payload_truncated: " << yes_no(record.payload_truncated) << "\n";
        return false;
    }
    return true;
}

template <typename T>
bool decode_payload(const yunlink::PacketTraceRecord& record, T* payload, std::ostringstream* ss) {
    if (!yunlink::decode_typed_payload(record.payload_preview, payload)) {
        *ss << "semantic decode failed for this payload.\n";
        return false;
    }
    *ss << "decoded_from_payload_preview: true\n";
    return true;
}

std::string command_kind_label(yunlink::CommandKind kind) {
    return packet_message_type_label(yunlink::MessageFamily::kCommand, static_cast<uint16_t>(kind));
}

void append_command(const yunlink::PacketTraceRecord& record, std::ostringstream* ss) {
    switch (static_cast<yunlink::CommandType>(record.envelope.message_type)) {
    case yunlink::CommandType::kTakeoff: {
        yunlink::TakeoffCommand payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "reserved: " << static_cast<int>(payload.reserved) << "\n";
        }
        break;
    }
    case yunlink::CommandType::kLand: {
        yunlink::LandCommand payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "reserved: " << static_cast<int>(payload.reserved) << "\n";
        }
        break;
    }
    case yunlink::CommandType::kReturn: {
        yunlink::ReturnCommand payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "reserved: " << static_cast<int>(payload.reserved) << "\n";
        }
        break;
    }
    case yunlink::CommandType::kGoto: {
        yunlink::GotoCommand payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "x_m: " << payload.x_m << "\n";
            *ss << "y_m: " << payload.y_m << "\n";
            *ss << "z_m: " << payload.z_m << "\n";
            *ss << "yaw_rad: " << payload.yaw_rad << "\n";
        }
        break;
    }
    case yunlink::CommandType::kVelocitySetpoint: {
        yunlink::VelocitySetpointCommand payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "vx_mps: " << payload.vx_mps << "\n";
            *ss << "vy_mps: " << payload.vy_mps << "\n";
            *ss << "vz_mps: " << payload.vz_mps << "\n";
            *ss << "yaw_rate_radps: " << payload.yaw_rate_radps << "\n";
            *ss << "body_frame: " << yes_no(payload.body_frame) << "\n";
        }
        break;
    }
    default:
        *ss << "semantic decode not implemented for this Command type.\n";
        break;
    }
}

void append_command_result(const yunlink::PacketTraceRecord& record, std::ostringstream* ss) {
    yunlink::CommandResult payload{};
    if (!decode_payload(record, &payload, ss)) {
        return;
    }
    *ss << "command_kind: " << command_kind_label(payload.command_kind) << "\n";
    *ss << "phase: " << static_cast<int>(payload.phase) << "\n";
    *ss << "result_code: " << payload.result_code << "\n";
    *ss << "progress_percent: " << static_cast<int>(payload.progress_percent) << "\n";
    *ss << "detail: " << payload.detail << "\n";
}

void append_command_status(const yunlink::PacketTraceRecord& record, std::ostringstream* ss) {
    yunlink::CommandExecutionStatusSnapshot payload{};
    if (!decode_payload(record, &payload, ss)) {
        return;
    }
    *ss << "agent_name: " << payload.agent_name << "\n";
    *ss << "agent_id: " << static_cast<int>(payload.agent_id) << "\n";
    *ss << "command_kind: " << command_kind_label(payload.command_kind) << "\n";
    *ss << "execution_state: " << static_cast<int>(payload.execution_state) << "\n";
    *ss << "progress_percent: " << static_cast<int>(payload.progress_percent) << "\n";
    *ss << "active: " << yes_no(payload.active) << "\n";
    *ss << "terminal: " << yes_no(payload.terminal) << "\n";
    *ss << "success: " << yes_no(payload.success) << "\n";
    *ss << "ready_for_takeoff: " << yes_no(payload.ready_for_takeoff) << "\n";
    *ss << "ready_for_land: " << yes_no(payload.ready_for_land) << "\n";
    *ss << "detail: " << payload.detail << "\n";
    *ss << "busy_reason: " << payload.busy_reason << "\n";
}

void append_string_list(const char* label,
                        const std::vector<std::string>& values,
                        std::ostringstream* ss) {
    *ss << label << "_count: " << values.size() << "\n";
    const size_t limit = std::min<size_t>(values.size(), 8);
    for (size_t i = 0; i < limit; ++i) {
        *ss << label << "[" << i << "]: " << values[i] << "\n";
    }
    if (values.size() > limit) {
        *ss << label << "_truncated: true\n";
    }
}

void append_feature_get_response(const yunlink::FeatureGetResponse& payload,
                                 std::ostringstream* ss) {
    *ss << "success: " << yes_no(payload.success) << "\n";
    *ss << "message: " << payload.message << "\n";
    *ss << "name: " << payload.name << "\n";
    *ss << "title: " << payload.title << "\n";
    *ss << "group: " << payload.group << "\n";
    *ss << "running: " << yes_no(payload.running) << "\n";
    *ss << "auto_start: " << yes_no(payload.auto_start) << "\n";
    append_string_list("depends_on", payload.depends_on, ss);
    append_string_list("start_preview_units", payload.start_preview_units, ss);
}

void append_system_service(const yunlink::PacketTraceRecord& record, std::ostringstream* ss) {
    switch (static_cast<yunlink::SystemServiceType>(record.envelope.message_type)) {
    case yunlink::SystemServiceType::kFeatureListRequest: {
        yunlink::FeatureListRequest payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "reserved: " << static_cast<int>(payload.reserved) << "\n";
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureListResponse: {
        yunlink::FeatureListResponse payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "success: " << yes_no(payload.success) << "\n";
            *ss << "message: " << payload.message << "\n";
            append_string_list("feature_names", payload.feature_names, ss);
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureGetRequest: {
        yunlink::FeatureGetRequest payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "feature_name: " << payload.feature_name << "\n";
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureGetResponse: {
        yunlink::FeatureGetResponse payload{};
        if (decode_payload(record, &payload, ss)) {
            append_feature_get_response(payload, ss);
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureStartRequest: {
        yunlink::FeatureStartRequest payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "feature_name: " << payload.feature_name << "\n";
            *ss << "restart_if_running: " << yes_no(payload.restart_if_running) << "\n";
            *ss << "start_with_terminal: " << yes_no(payload.start_with_terminal) << "\n";
            append_string_list("override_args", payload.override_args, ss);
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureStartResponse: {
        yunlink::FeatureStartResponse payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "success: " << yes_no(payload.success) << "\n";
            *ss << "message: " << payload.message << "\n";
            *ss << "feature_name: " << payload.feature_name << "\n";
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureStopRequest: {
        yunlink::FeatureStopRequest payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "feature_name: " << payload.feature_name << "\n";
            *ss << "force: " << yes_no(payload.force) << "\n";
        }
        break;
    }
    case yunlink::SystemServiceType::kFeatureStopResponse: {
        yunlink::FeatureStopResponse payload{};
        if (decode_payload(record, &payload, ss)) {
            *ss << "success: " << yes_no(payload.success) << "\n";
            *ss << "message: " << payload.message << "\n";
            *ss << "feature_name: " << payload.feature_name << "\n";
        }
        break;
    }
    }
}

}  // namespace

std::string packet_semantic_detail(const yunlink::PacketTraceRecord& record) {
    std::ostringstream ss;
    if (!record.has_envelope) {
        return "Semantic decode waits for a decoded SecureEnvelope.";
    }
    ss << "semantic family: " << packet_family_label(record.envelope.message_family) << "\n";
    ss << "semantic type: "
       << packet_message_type_label(record.envelope.message_family, record.envelope.message_type)
       << "\n";
    if (!payload_available(record, &ss)) {
        return ss.str();
    }
    switch (record.envelope.message_family) {
    case yunlink::MessageFamily::kCommand:
        append_command(record, &ss);
        break;
    case yunlink::MessageFamily::kCommandResult:
        append_command_result(record, &ss);
        break;
    case yunlink::MessageFamily::kStateSnapshot:
        if (static_cast<yunlink::StateSnapshotType>(record.envelope.message_type) ==
            yunlink::StateSnapshotType::kCommandExecutionStatus) {
            append_command_status(record, &ss);
        } else {
            ss << "semantic decode not implemented for this StateSnapshot type.\n";
        }
        break;
    case yunlink::MessageFamily::kSystemService:
        append_system_service(record, &ss);
        break;
    case yunlink::MessageFamily::kConfigurationService:
        append_configuration_trace(record, &ss);
        break;
    default:
        ss << "semantic decode not implemented for this family.\n";
        break;
    }
    return ss.str();
}
