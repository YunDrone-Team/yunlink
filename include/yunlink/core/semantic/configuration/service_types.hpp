/**
 * @file include/yunlink/core/semantic/configuration/service_types.hpp
 * @brief Provider-neutral typed configuration resource models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_CONFIGURATION_SERVICE_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_CONFIGURATION_SERVICE_TYPES_HPP

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace yunlink {

enum class ConfigValueType : uint8_t {
    kBool = 1,
    kInt64 = 2,
    kDouble = 3,
    kString = 4,
    kStringList = 5,
};

enum class ConfigServiceStatus : uint8_t {
    kOk = 0,
    kNotFound = 1,
    kUnsupported = 2,
    kUnauthenticated = 3,
    kUnauthorized = 4,
    kConflict = 5,
    kInvalid = 6,
    kUnsafeState = 7,
    kInternalError = 8,
};

enum class ConfigApplyRequirement : uint8_t {
    kNone = 0,
    kComponentRestart = 1,
    kEndpointRestart = 2,
    kDeviceReboot = 3,
    kManual = 4,
};

enum class ConfigApplyOutcome : uint8_t {
    kApplied = 1,
    kRestartScheduled = 2,
    kManualActionRequired = 3,
    kFailed = 4,
};

struct ConfigValue {
    ConfigValueType type = ConfigValueType::kString;
    bool bool_value = false;
    int64_t int64_value = 0;
    double double_value = 0.0;
    std::string string_value;
    std::vector<std::string> string_list_value;

    static ConfigValue from_bool(bool value) {
        ConfigValue out;
        out.type = ConfigValueType::kBool;
        out.bool_value = value;
        return out;
    }
    static ConfigValue from_int64(int64_t value) {
        ConfigValue out;
        out.type = ConfigValueType::kInt64;
        out.int64_value = value;
        return out;
    }
    static ConfigValue from_double(double value) {
        ConfigValue out;
        out.type = ConfigValueType::kDouble;
        out.double_value = value;
        return out;
    }
    static ConfigValue from_string(std::string value) {
        ConfigValue out;
        out.type = ConfigValueType::kString;
        out.string_value = std::move(value);
        return out;
    }
    static ConfigValue from_string_list(std::vector<std::string> value) {
        ConfigValue out;
        out.type = ConfigValueType::kStringList;
        out.string_list_value = std::move(value);
        return out;
    }
};

struct ConfigResourceDescriptor {
    std::string id;
    std::string title;
    std::string description;
    bool readable = true;
    bool writable = false;
    bool apply_supported = false;
};

struct ConfigChoice {
    ConfigValue value;
    std::string label;
};

struct ConfigFieldSchema {
    std::string path;
    std::string title;
    std::string description;
    ConfigValueType type = ConfigValueType::kString;
    bool required = false;
    bool read_only = false;
    bool sensitive = false;
    bool has_minimum = false;
    double minimum = 0.0;
    bool has_maximum = false;
    double maximum = 0.0;
    std::string validation_pattern;
    std::vector<ConfigChoice> choices;
};

struct ConfigFieldValue {
    std::string path;
    ConfigValue value;
};

struct ConfigSnapshot {
    std::string resource_id;
    std::string revision;
    std::string applied_revision;
    std::vector<ConfigFieldValue> values;
};

struct ConfigFieldError {
    std::string path;
    std::string code;
    std::string message;
};

struct ConfigEffects {
    ConfigApplyRequirement requirement = ConfigApplyRequirement::kNone;
    std::vector<std::string> affected_components;
    bool reconnect_expected = false;
};

struct ConfigResourceListRequest {
    uint8_t reserved = 0;
};

struct ConfigResourceListResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    std::vector<ConfigResourceDescriptor> resources;
};

struct ConfigResourceDescribeRequest {
    std::string resource_id;
};

struct ConfigResourceDescribeResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigResourceDescriptor resource;
    std::vector<ConfigFieldSchema> fields;
};

struct ConfigResourceGetRequest {
    std::string resource_id;
};

struct ConfigResourceGetResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigSnapshot snapshot;
};

struct ConfigResourcePatchRequest {
    std::string resource_id;
    std::string expected_revision;
    std::vector<ConfigFieldValue> updates;
    bool validate_only = false;
};

struct ConfigResourcePatchResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigSnapshot snapshot;
    std::vector<ConfigFieldError> errors;
    ConfigEffects effects;
};

struct ConfigResourceApplyRequest {
    std::string resource_id;
    std::string expected_revision;
};

struct ConfigResourceApplyResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    std::string applied_revision;
    ConfigApplyOutcome outcome = ConfigApplyOutcome::kFailed;
    ConfigEffects effects;
};

struct ConfigurationServiceHandle {
    uint64_t message_id = 0;
    uint64_t session_id = 0;
    uint64_t created_at_ms = 0;
    uint32_t ttl_ms = 0;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_CONFIGURATION_SERVICE_TYPES_HPP
