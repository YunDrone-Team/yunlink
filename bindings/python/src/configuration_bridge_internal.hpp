#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "configuration_bridge.hpp"

namespace yunlink_python_config {

using ValueData = std::variant<bool, int64_t, double, std::string, std::vector<std::string>>;

struct OwnedValue {
    uint8_t type = YUNLINK_CONFIG_VALUE_STRING;
    ValueData data = std::string();
};

struct OwnedDescriptor {
    std::string id;
    std::string title;
    std::string description;
    bool readable = false;
    bool writable = false;
    bool apply_supported = false;
};

struct OwnedChoice {
    OwnedValue value;
    std::string label;
};

struct OwnedFieldSchema {
    std::string path;
    std::string title;
    std::string description;
    uint8_t type = 0;
    bool required = false;
    bool read_only = false;
    bool sensitive = false;
    bool has_minimum = false;
    double minimum = 0.0;
    bool has_maximum = false;
    double maximum = 0.0;
    std::string validation_pattern;
    std::vector<OwnedChoice> choices;
};

struct OwnedFieldValue {
    std::string path;
    OwnedValue value;
};

struct OwnedSnapshot {
    std::string resource_id;
    std::string revision;
    std::string applied_revision;
    std::vector<OwnedFieldValue> values;
};

struct OwnedFieldError {
    std::string path;
    std::string code;
    std::string message;
};

struct OwnedEffects {
    uint8_t requirement = 0;
    std::vector<std::string> affected_components;
    bool reconnect_expected = false;
};

struct OwnedResponse {
    std::string kind;
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    uint8_t status = YUNLINK_CONFIG_STATUS_INTERNAL_ERROR;
    std::string message;
    std::vector<OwnedDescriptor> resources;
    OwnedDescriptor resource;
    std::vector<OwnedFieldSchema> fields;
    OwnedSnapshot snapshot;
    std::vector<OwnedFieldError> errors;
    OwnedEffects effects;
    std::string applied_revision;
    uint8_t outcome = YUNLINK_CONFIG_OUTCOME_FAILED;
};

std::string copyView(yunlink_string_view_t view);

template <typename T> const T* checkedArray(const T* values, size_t count) {
    if (count != 0 && values == nullptr) {
        throw std::runtime_error("invalid YunLink array view");
    }
    return values;
}

OwnedValue copyValue(const yunlink_config_value_view_t& view);
OwnedDescriptor copyDescriptor(const yunlink_config_resource_descriptor_view_t& view);
OwnedSnapshot copySnapshot(const yunlink_config_snapshot_view_t& view);
OwnedEffects copyEffects(const yunlink_config_effects_view_t& view);
nanobind::dict responseToPython(const OwnedResponse& value);
void throwIfError(yunlink_result_t result);

}  // namespace yunlink_python_config

struct ConfigurationBridge::Impl {
    std::mutex mutex;
    std::deque<yunlink_python_config::OwnedResponse> responses;
    std::vector<size_t> tokens;

    void push(yunlink_python_config::OwnedResponse response) noexcept;
};
