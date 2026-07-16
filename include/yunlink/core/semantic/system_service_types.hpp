/**
 * @file include/yunlink/core/semantic/system_service_types.hpp
 * @brief Semantic system service payload models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP

#include <cstdint>
#include <string>
#include <vector>

#include "yunlink/core/types.hpp"

namespace yunlink {

struct FeatureListRequest {
    uint8_t reserved = 0;
};

struct FeatureListResponse {
    bool success = false;
    std::string message;
    std::vector<std::string> feature_names;
};

struct FeatureGetRequest {
    std::string feature_name;
};

struct FeatureGetResponse {
    bool success = false;
    std::string message;
    std::string name;
    std::string title;
    std::string group;
    bool running = false;
    std::string description;
    bool auto_start = false;
    std::vector<std::string> depends_on;
    std::vector<std::string> start_preview_units;
    std::vector<std::string> start_preview_commands;
};

struct FeatureStartRequest {
    std::string feature_name;
    std::vector<std::string> override_args;
    bool restart_if_running = false;
    bool start_with_terminal = false;
};

struct FeatureStartResponse {
    bool success = false;
    std::string message;
    std::string feature_name;
};

struct FeatureStopRequest {
    std::string feature_name;
    bool force = false;
};

struct FeatureStopResponse {
    bool success = false;
    std::string message;
    std::string feature_name;
};

struct RuntimeLogSummary {
    std::string runtime_id;
    std::string feature_name;
    std::string title;
    std::string state;
    uint64_t started_at_ns = 0;
    uint64_t finished_at_ns = 0;
    bool has_exit_code = false;
    int32_t exit_code = 0;
    std::string message;
};

struct RuntimeLogListRequest {
    uint8_t reserved = 0;
};

struct RuntimeLogListResponse {
    bool success = false;
    std::string message;
    std::vector<RuntimeLogSummary> runtimes;
};

struct RuntimeLogReadRequest {
    std::string runtime_id;
    uint64_t cursor = 0;
    uint32_t max_bytes = 0;
};

struct RuntimeLogReadResponse {
    bool success = false;
    std::string message;
    std::string runtime_id;
    std::string chunk;
    uint64_t next_cursor = 0;
    bool truncated = false;
    bool eof = false;
};

struct SystemServiceHandle {
    uint64_t session_id = 0;
    uint64_t message_id = 0;
    uint64_t correlation_id = 0;
    TargetSelector target;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_SYSTEM_SERVICE_TYPES_HPP
