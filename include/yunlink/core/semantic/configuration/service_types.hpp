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
    kDoubleList = 6,
    // 删除该路径的 provider 侧覆写，回落到更低层。它是一条"写入指令"，不是一种可持久化的
    // 字段类型：schema 的 type 不应出现该取值，只有 ConfigResourcePatchRequest.updates
    // 里的 ConfigValue 会用它（无 payload，只编码类型字节）。
    // 必须追加在末尾：不能改动已有取值，协议里是裸数字。
    kUnset = 7,
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

// Describes when a persisted field change can take effect.  This is intentionally
// provider-neutral: adapters translate their local lifecycle terminology here.
enum class ConfigFieldUpdatePolicy : uint8_t {
    kHotReload = 0,
    kComponentRestart = 1,
    kEndpointRestart = 2,
    kDeviceReboot = 3,
    kManual = 4,
    // 保存后需要重新生成并编译产物才会生效；保存本身不会失败，也不需要重启。
    // 注意这一档**不是"更重的重启"**：重启进程只要几秒，重新 codegen + 编译是数十分钟
    // 且需要停机，客户端必须把两者显示成不同的话术。
    // 必须追加在末尾：不能改动已有取值，协议里是裸数字。
    kRebuildRequired = 5,
};

enum class ConfigVariantSource : uint8_t {
    kDefault = 1,
    kActive = 2,
};

struct ConfigValue {
    ConfigValueType type = ConfigValueType::kString;
    bool bool_value = false;
    int64_t int64_value = 0;
    double double_value = 0.0;
    std::string string_value;
    std::vector<std::string> string_list_value;
    std::vector<double> double_list_value;

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
    static ConfigValue from_double_list(std::vector<double> value) {
        ConfigValue out;
        out.type = ConfigValueType::kDoubleList;
        out.double_list_value = std::move(value);
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
    bool variants_supported = false;
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
    std::string group_path;
    ConfigFieldUpdatePolicy update_policy = ConfigFieldUpdatePolicy::kManual;
    std::string unit;
    // 该字段的默认值（provider 侧语义）。has_default_value 为 false 表示 provider 不提供
    // 默认值，此时客户端不应显示"默认值"，也不应提供"重置为默认值"入口。
    // 注意不能靠 default_value 本身是否为空来判断：默认值可能是空串、0 或 false。
    ConfigValue default_value;
    bool has_default_value = false;
    // true 表示高级参数：地面站只在专家模式下显示。缺省 false（普通参数，标准模式也显示）。
    // 只影响显示，不影响可写性 / 校验 / 存储 / 生效策略，也不是权限控制。
    bool advanced = false;
};

struct ConfigFieldValue {
    std::string path;
    ConfigValue value;
};

struct ConfigSnapshot {
    std::string resource_id;
    std::string revision;
    std::string applied_revision;
    std::string variant_id;
    std::string active_variant_id;
    std::vector<ConfigFieldValue> values;
    // 该资源 + 该方案下被用户覆写（provider 侧稀疏覆写里存在、且不是"删除"语义）的字段
    // 路径。不在列表里的字段，其有效值等于默认值（或机型覆写值），用户没有改过。
    // provider 不提供该信息时保持为空，客户端应把所有字段当作"未覆写"，而不是猜测。
    std::vector<std::string> user_overridden_paths;
};

struct ConfigVariantDescriptor {
    std::string id;
    std::string title;
    std::string revision;
    uint64_t modified_at_ns = 0;
    bool active = false;
    bool mutable_variant = true;
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
    // 地面站 UI 语言（BCP-47，例如 zh-CN / en-US / uk-UA）；空表示由 provider 取默认。
    // provider 按它解析 display_name / description / 枚举标签。
    std::string locale;
};

struct ConfigResourceDescribeResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigResourceDescriptor resource;
    std::vector<ConfigFieldSchema> fields;
};

struct ConfigResourceGetRequest {
    std::string resource_id;
    // Empty selects the resource's currently active variant.
    std::string variant_id;
};

struct ConfigResourceGetResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigSnapshot snapshot;
};

struct ConfigResourcePatchRequest {
    std::string resource_id;
    // Empty selects the resource's currently active variant.
    std::string variant_id;
    std::string expected_revision;
    std::vector<ConfigFieldValue> updates;
    bool validate_only = false;
};

struct ConfigResourcePatchResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    // Current persisted snapshot. For validate-only patches this remains the exact
    // base snapshot that the client supplied in expected_revision.
    ConfigSnapshot snapshot;
    std::vector<ConfigFieldError> errors;
    ConfigEffects effects;
    // Provider-normalized proposal for a successful validate-only patch. This is
    // display-only: candidate_snapshot.revision identifies candidate content, not a
    // persisted revision accepted by a later save. A client saves by resubmitting the
    // same updates with snapshot.revision; a concurrent persisted change then yields
    // Conflict and requires a fresh Get + validation cycle.
    bool has_candidate_snapshot = false;
    ConfigSnapshot candidate_snapshot;
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

struct ConfigResourceVariantListRequest {
    std::string resource_id;
};

struct ConfigResourceVariantListResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    std::string active_variant_id;
    std::vector<ConfigVariantDescriptor> variants;
};

struct ConfigResourceVariantCreateRequest {
    std::string resource_id;
    std::string variant_id;
    ConfigVariantSource source = ConfigVariantSource::kActive;
    std::string expected_active_revision;
};

struct ConfigResourceVariantCreateResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigVariantDescriptor variant;
    ConfigEffects effects;
};

struct ConfigResourceVariantSaveCurrentRequest {
    std::string resource_id;
    std::string variant_id;
    std::string expected_variant_revision;
    std::string expected_active_revision;
};

struct ConfigResourceVariantSaveCurrentResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    ConfigVariantDescriptor variant;
    ConfigEffects effects;
};

struct ConfigResourceVariantActivateRequest {
    std::string resource_id;
    std::string variant_id;
    std::string expected_active_revision;
};

struct ConfigResourceVariantActivateResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
    std::string applied_revision;
    ConfigApplyOutcome outcome = ConfigApplyOutcome::kFailed;
    ConfigEffects effects;
};

struct ConfigResourceVariantDeleteRequest {
    std::string resource_id;
    std::string variant_id;
    std::string expected_revision;
};

struct ConfigResourceVariantDeleteResponse {
    ConfigServiceStatus status = ConfigServiceStatus::kInternalError;
    std::string message;
};

struct ConfigurationServiceHandle {
    uint64_t message_id = 0;
    uint64_t session_id = 0;
    uint64_t created_at_ms = 0;
    uint32_t ttl_ms = 0;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_CONFIGURATION_SERVICE_TYPES_HPP
