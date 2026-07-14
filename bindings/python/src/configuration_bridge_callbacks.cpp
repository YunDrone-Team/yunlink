#include "configuration_bridge_internal.hpp"

using namespace yunlink_python_config;

void ConfigurationBridge::onList(
    void* user_data,
    const yunlink_config_resource_list_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        OwnedResponse output;
        output.kind = "configuration_list";
        output.session_id = response->session_id;
        output.message_id = response->message_id;
        output.correlation_id = response->correlation_id;
        output.status = response->status;
        output.message = copyView(response->message);
        const auto* resources = checkedArray(response->resources, response->resource_count);
        for (size_t index = 0; index < response->resource_count; ++index) {
            output.resources.push_back(copyDescriptor(resources[index]));
        }
        static_cast<ConfigurationBridge*>(user_data)->impl_->push(std::move(output));
    } catch (...) {
    }
}

void ConfigurationBridge::onDescribe(
    void* user_data,
    const yunlink_config_resource_describe_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        OwnedResponse output;
        output.kind = "configuration_describe";
        output.session_id = response->session_id;
        output.message_id = response->message_id;
        output.correlation_id = response->correlation_id;
        output.status = response->status;
        output.message = copyView(response->message);
        output.resource = copyDescriptor(response->resource);
        const auto* fields = checkedArray(response->fields, response->field_count);
        for (size_t index = 0; index < response->field_count; ++index) {
            const auto& source = fields[index];
            OwnedFieldSchema field;
            field.path = copyView(source.path);
            field.title = copyView(source.title);
            field.description = copyView(source.description);
            field.type = source.type;
            field.required = source.required != 0;
            field.read_only = source.read_only != 0;
            field.sensitive = source.sensitive != 0;
            field.has_minimum = source.has_minimum != 0;
            field.minimum = source.minimum;
            field.has_maximum = source.has_maximum != 0;
            field.maximum = source.maximum;
            field.validation_pattern = copyView(source.validation_pattern);
            const auto* choices = checkedArray(source.choices, source.choice_count);
            for (size_t choice_index = 0; choice_index < source.choice_count; ++choice_index) {
                field.choices.push_back({copyValue(choices[choice_index].value),
                                         copyView(choices[choice_index].label)});
            }
            output.fields.push_back(std::move(field));
        }
        static_cast<ConfigurationBridge*>(user_data)->impl_->push(std::move(output));
    } catch (...) {
    }
}

void ConfigurationBridge::onGet(
    void* user_data,
    const yunlink_config_resource_get_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        OwnedResponse output;
        output.kind = "configuration_get";
        output.session_id = response->session_id;
        output.message_id = response->message_id;
        output.correlation_id = response->correlation_id;
        output.status = response->status;
        output.message = copyView(response->message);
        output.snapshot = copySnapshot(response->snapshot);
        static_cast<ConfigurationBridge*>(user_data)->impl_->push(std::move(output));
    } catch (...) {
    }
}

void ConfigurationBridge::onPatch(
    void* user_data,
    const yunlink_config_resource_patch_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        OwnedResponse output;
        output.kind = "configuration_patch";
        output.session_id = response->session_id;
        output.message_id = response->message_id;
        output.correlation_id = response->correlation_id;
        output.status = response->status;
        output.message = copyView(response->message);
        output.snapshot = copySnapshot(response->snapshot);
        output.effects = copyEffects(response->effects);
        const auto* errors = checkedArray(response->errors, response->error_count);
        for (size_t index = 0; index < response->error_count; ++index) {
            output.errors.push_back({copyView(errors[index].path),
                                     copyView(errors[index].code),
                                     copyView(errors[index].message)});
        }
        static_cast<ConfigurationBridge*>(user_data)->impl_->push(std::move(output));
    } catch (...) {
    }
}

void ConfigurationBridge::onApply(
    void* user_data,
    const yunlink_config_resource_apply_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        OwnedResponse output;
        output.kind = "configuration_apply";
        output.session_id = response->session_id;
        output.message_id = response->message_id;
        output.correlation_id = response->correlation_id;
        output.status = response->status;
        output.message = copyView(response->message);
        output.applied_revision = copyView(response->applied_revision);
        output.outcome = response->outcome;
        output.effects = copyEffects(response->effects);
        static_cast<ConfigurationBridge*>(user_data)->impl_->push(std::move(output));
    } catch (...) {
    }
}
