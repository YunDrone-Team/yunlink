#include "packets/format/configuration_trace_format.hpp"

#include <yunlink/core/semantic_messages.hpp>

namespace {

template <typename T>
bool decode(const yunlink::PacketTraceRecord& record, T* payload, std::ostringstream* output) {
    if (yunlink::decode_typed_payload(record.payload_preview, payload)) {
        return true;
    }
    *output << "semantic decode failed for this payload.\n";
    return false;
}

const char* yes_no(bool value) {
    return value ? "true" : "false";
}

}  // namespace

void append_configuration_trace(const yunlink::PacketTraceRecord& record,
                                std::ostringstream* output) {
    const auto append_status = [output](yunlink::ConfigServiceStatus status,
                                        const std::string& message) {
        *output << "status: " << static_cast<int>(status) << "\n";
        *output << "message: " << message << "\n";
    };
    switch (static_cast<yunlink::ConfigurationServiceType>(record.envelope.message_type)) {
    case yunlink::ConfigurationServiceType::kResourceListRequest: {
        yunlink::ConfigResourceListRequest payload{};
        if (decode(record, &payload, output)) {
            *output << "reserved: " << static_cast<int>(payload.reserved) << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceListResponse: {
        yunlink::ConfigResourceListResponse payload{};
        if (decode(record, &payload, output)) {
            append_status(payload.status, payload.message);
            *output << "resource_count: " << payload.resources.size() << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceDescribeRequest: {
        yunlink::ConfigResourceDescribeRequest payload{};
        if (decode(record, &payload, output)) {
            *output << "resource_id: " << payload.resource_id << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceDescribeResponse: {
        yunlink::ConfigResourceDescribeResponse payload{};
        if (decode(record, &payload, output)) {
            append_status(payload.status, payload.message);
            *output << "resource_id: " << payload.resource.id << "\n";
            *output << "field_count: " << payload.fields.size() << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceGetRequest: {
        yunlink::ConfigResourceGetRequest payload{};
        if (decode(record, &payload, output)) {
            *output << "resource_id: " << payload.resource_id << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceGetResponse: {
        yunlink::ConfigResourceGetResponse payload{};
        if (decode(record, &payload, output)) {
            append_status(payload.status, payload.message);
            *output << "resource_id: " << payload.snapshot.resource_id << "\n";
            *output << "revision: " << payload.snapshot.revision << "\n";
            *output << "applied_revision: " << payload.snapshot.applied_revision << "\n";
            *output << "value_count: " << payload.snapshot.values.size() << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourcePatchRequest: {
        yunlink::ConfigResourcePatchRequest payload{};
        if (decode(record, &payload, output)) {
            *output << "resource_id: " << payload.resource_id << "\n";
            *output << "expected_revision: " << payload.expected_revision << "\n";
            *output << "update_count: " << payload.updates.size() << "\n";
            *output << "validate_only: " << yes_no(payload.validate_only) << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourcePatchResponse: {
        yunlink::ConfigResourcePatchResponse payload{};
        if (decode(record, &payload, output)) {
            append_status(payload.status, payload.message);
            *output << "revision: " << payload.snapshot.revision << "\n";
            *output << "error_count: " << payload.errors.size() << "\n";
            *output << "reconnect_expected: " << yes_no(payload.effects.reconnect_expected) << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceApplyRequest: {
        yunlink::ConfigResourceApplyRequest payload{};
        if (decode(record, &payload, output)) {
            *output << "resource_id: " << payload.resource_id << "\n";
            *output << "expected_revision: " << payload.expected_revision << "\n";
        }
        break;
    }
    case yunlink::ConfigurationServiceType::kResourceApplyResponse: {
        yunlink::ConfigResourceApplyResponse payload{};
        if (decode(record, &payload, output)) {
            append_status(payload.status, payload.message);
            *output << "applied_revision: " << payload.applied_revision << "\n";
            *output << "outcome: " << static_cast<int>(payload.outcome) << "\n";
            *output << "reconnect_expected: " << yes_no(payload.effects.reconnect_expected) << "\n";
        }
        break;
    }
    }
}
