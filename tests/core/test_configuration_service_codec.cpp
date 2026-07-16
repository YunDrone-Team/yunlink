/**
 * @file tests/core/test_configuration_service_codec.cpp
 * @brief Configuration resource schema-1 codec contract.
 */

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "yunlink/core/semantic_messages.hpp"

namespace {

bool same_value(const yunlink::ConfigValue& lhs, const yunlink::ConfigValue& rhs) {
    return lhs.type == rhs.type && lhs.bool_value == rhs.bool_value &&
           lhs.int64_value == rhs.int64_value &&
           std::abs(lhs.double_value - rhs.double_value) < 1e-9 &&
           lhs.string_value == rhs.string_value && lhs.string_list_value == rhs.string_list_value &&
           lhs.double_list_value == rhs.double_list_value;
}

template <typename T> bool roundtrip(const T& source, T* decoded) {
    const yunlink::ByteBuffer encoded = yunlink::encode_typed_payload(source);
    return !encoded.empty() && yunlink::decode_typed_payload(encoded, decoded);
}

bool test_value_types_and_snapshot() {
    yunlink::ConfigResourceGetResponse source{};
    source.status = yunlink::ConfigServiceStatus::kOk;
    source.message = "ok";
    source.snapshot.resource_id = "vendor.device.identity";
    source.snapshot.revision = "sha256:stored";
    source.snapshot.applied_revision = "sha256:active";
    source.snapshot.values = {
        {"enabled", yunlink::ConfigValue::from_bool(true)},
        {"agent_id", yunlink::ConfigValue::from_int64(7)},
        {"gain", yunlink::ConfigValue::from_double(1.25)},
        {"name", yunlink::ConfigValue::from_string("uav")},
        {"profile_dirs",
         yunlink::ConfigValue::from_string_list({"~/.profiles", "/opt/vendor/profiles"})},
        {"coefficients", yunlink::ConfigValue::from_double_list({1.0, 0.5, 0.25})},
    };

    yunlink::ConfigResourceGetResponse decoded{};
    if (!roundtrip(source, &decoded) || decoded.status != source.status ||
        decoded.message != source.message ||
        decoded.snapshot.resource_id != source.snapshot.resource_id ||
        decoded.snapshot.revision != source.snapshot.revision ||
        decoded.snapshot.applied_revision != source.snapshot.applied_revision ||
        decoded.snapshot.values.size() != source.snapshot.values.size()) {
        return false;
    }
    for (std::size_t index = 0; index < source.snapshot.values.size(); ++index) {
        if (decoded.snapshot.values[index].path != source.snapshot.values[index].path ||
            !same_value(decoded.snapshot.values[index].value,
                        source.snapshot.values[index].value)) {
            return false;
        }
    }
    return true;
}

bool test_schema_roundtrip() {
    yunlink::ConfigResourceDescribeResponse source{};
    source.status = yunlink::ConfigServiceStatus::kOk;
    source.message = "ok";
    source.resource.id = "vendor.device.identity";
    source.resource.title = "Device identity";
    source.resource.description = "Stable endpoint identity";
    source.resource.readable = true;
    source.resource.writable = true;
    source.resource.apply_supported = true;

    yunlink::ConfigFieldSchema field{};
    field.path = "agent_id";
    field.title = "Agent ID";
    field.description = "Non-negative endpoint identifier";
    field.type = yunlink::ConfigValueType::kInt64;
    field.required = true;
    field.read_only = false;
    field.sensitive = false;
    field.has_minimum = true;
    field.minimum = 0.0;
    field.has_maximum = true;
    field.maximum = 4294967295.0;
    field.validation_pattern = "^[0-9]+$";
    field.choices.push_back({yunlink::ConfigValue::from_int64(1), "Primary"});
    source.fields.push_back(field);

    yunlink::ConfigResourceDescribeResponse decoded{};
    if (!roundtrip(source, &decoded) || decoded.resource.id != source.resource.id ||
        decoded.fields.size() != 1) {
        return false;
    }
    const auto& actual = decoded.fields.front();
    return actual.path == field.path && actual.type == field.type && actual.required &&
           actual.has_minimum && actual.minimum == field.minimum && actual.has_maximum &&
           actual.maximum == field.maximum &&
           actual.validation_pattern == field.validation_pattern && actual.choices.size() == 1 &&
           same_value(actual.choices.front().value, field.choices.front().value);
}

bool test_all_operations_roundtrip() {
    yunlink::ConfigResourceListRequest list_request{};
    yunlink::ConfigResourceListRequest decoded_list_request{};
    if (!roundtrip(list_request, &decoded_list_request)) {
        return false;
    }

    yunlink::ConfigResourceListResponse list_response{};
    list_response.status = yunlink::ConfigServiceStatus::kOk;
    list_response.message = "ok";
    list_response.resources.push_back(
        {"vendor.device.identity", "Identity", "Endpoint identity", true, true, true});
    yunlink::ConfigResourceListResponse decoded_list_response{};
    if (!roundtrip(list_response, &decoded_list_response) ||
        decoded_list_response.resources.size() != 1) {
        return false;
    }

    yunlink::ConfigResourceDescribeRequest describe_request{"vendor.device.identity"};
    yunlink::ConfigResourceDescribeRequest decoded_describe_request{};
    if (!roundtrip(describe_request, &decoded_describe_request) ||
        decoded_describe_request.resource_id != describe_request.resource_id) {
        return false;
    }

    yunlink::ConfigResourceGetRequest get_request{"vendor.device.identity"};
    yunlink::ConfigResourceGetRequest decoded_get_request{};
    if (!roundtrip(get_request, &decoded_get_request) ||
        decoded_get_request.resource_id != get_request.resource_id) {
        return false;
    }

    yunlink::ConfigResourcePatchRequest patch_request{};
    patch_request.resource_id = "vendor.device.identity";
    patch_request.expected_revision = "sha256:before";
    patch_request.updates.push_back({"agent_id", yunlink::ConfigValue::from_int64(8)});
    patch_request.validate_only = true;
    yunlink::ConfigResourcePatchRequest decoded_patch_request{};
    if (!roundtrip(patch_request, &decoded_patch_request) ||
        decoded_patch_request.expected_revision != patch_request.expected_revision ||
        decoded_patch_request.updates.size() != 1 || !decoded_patch_request.validate_only) {
        return false;
    }

    yunlink::ConfigResourcePatchResponse patch_response{};
    patch_response.status = yunlink::ConfigServiceStatus::kInvalid;
    patch_response.message = "validation failed";
    patch_response.snapshot.resource_id = "vendor.device.identity";
    patch_response.snapshot.revision = "sha256:before";
    patch_response.errors.push_back({"agent_id", "range", "must be non-negative"});
    patch_response.effects.requirement = yunlink::ConfigApplyRequirement::kComponentRestart;
    patch_response.effects.affected_components = {"bridge", "controller"};
    patch_response.effects.reconnect_expected = true;
    yunlink::ConfigResourcePatchResponse decoded_patch_response{};
    if (!roundtrip(patch_response, &decoded_patch_response) ||
        decoded_patch_response.errors.size() != 1 ||
        decoded_patch_response.effects.affected_components.size() != 2 ||
        !decoded_patch_response.effects.reconnect_expected) {
        return false;
    }

    yunlink::ConfigResourceApplyRequest apply_request{};
    apply_request.resource_id = "vendor.device.identity";
    apply_request.expected_revision = "sha256:stored";
    yunlink::ConfigResourceApplyRequest decoded_apply_request{};
    if (!roundtrip(apply_request, &decoded_apply_request) ||
        decoded_apply_request.expected_revision != apply_request.expected_revision) {
        return false;
    }

    yunlink::ConfigResourceApplyResponse apply_response{};
    apply_response.status = yunlink::ConfigServiceStatus::kOk;
    apply_response.message = "restart scheduled";
    apply_response.applied_revision = "sha256:stored";
    apply_response.outcome = yunlink::ConfigApplyOutcome::kRestartScheduled;
    apply_response.effects.requirement = yunlink::ConfigApplyRequirement::kComponentRestart;
    apply_response.effects.reconnect_expected = true;
    yunlink::ConfigResourceApplyResponse decoded_apply_response{};
    return roundtrip(apply_response, &decoded_apply_response) &&
           decoded_apply_response.applied_revision == apply_response.applied_revision &&
           decoded_apply_response.outcome == apply_response.outcome &&
           decoded_apply_response.effects.reconnect_expected;
}

bool test_invalid_and_truncated_values_are_rejected() {
    yunlink::ConfigResourceGetResponse response{};
    response.status = yunlink::ConfigServiceStatus::kOk;
    response.snapshot.resource_id = "vendor.device.identity";
    response.snapshot.revision = "r1";
    response.snapshot.values.push_back({"name", yunlink::ConfigValue::from_string("uav")});

    yunlink::ByteBuffer encoded = yunlink::encode_typed_payload(response);
    if (encoded.size() < 2) {
        return false;
    }
    encoded.pop_back();
    yunlink::ConfigResourceGetResponse decoded{};
    if (yunlink::decode_typed_payload(encoded, &decoded)) {
        return false;
    }

    const yunlink::ByteBuffer invalid_value = {
        0,  // status: ok
        0,
        0,  // message
        0,
        0,  // resource_id
        0,
        0,  // revision
        0,
        0,  // applied_revision
        1,
        0,  // one field
        0,
        0,     // empty field path
        0xff,  // invalid ConfigValueType
    };
    return !yunlink::decode_typed_payload(invalid_value, &decoded);
}

bool test_oversized_values_are_rejected_without_truncation() {
    yunlink::ConfigResourcePatchRequest too_many_updates{};
    too_many_updates.resource_id = "vendor.device.identity";
    too_many_updates.expected_revision = "r1";
    for (int index = 0; index < 257; ++index) {
        too_many_updates.updates.push_back(
            {"field_" + std::to_string(index), yunlink::ConfigValue::from_int64(index)});
    }
    if (!yunlink::encode_typed_payload(too_many_updates).empty()) {
        return false;
    }

    yunlink::ConfigResourceDescribeRequest oversized_resource_id{};
    oversized_resource_id.resource_id.assign(1025, 'x');
    return yunlink::encode_typed_payload(oversized_resource_id).empty();
}

}  // namespace

int main() {
    if (!test_value_types_and_snapshot()) {
        std::cerr << "configuration value/snapshot roundtrip failed\n";
        return 1;
    }
    if (!test_schema_roundtrip()) {
        std::cerr << "configuration schema roundtrip failed\n";
        return 2;
    }
    if (!test_all_operations_roundtrip()) {
        std::cerr << "configuration operation roundtrip failed\n";
        return 3;
    }
    if (!test_invalid_and_truncated_values_are_rejected()) {
        std::cerr << "invalid/truncated configuration payload accepted\n";
        return 4;
    }
    if (!test_oversized_values_are_rejected_without_truncation()) {
        std::cerr << "oversized configuration value was silently truncated\n";
        return 5;
    }
    if (yunlink::MessageTraits<yunlink::ConfigResourcePatchRequest>::kSchemaVersion != 1 ||
        yunlink::MessageTraits<yunlink::ConfigResourcePatchRequest>::kFamily !=
            yunlink::MessageFamily::kConfigurationService) {
        std::cerr << "configuration message trait mismatch\n";
        return 6;
    }
    return 0;
}
