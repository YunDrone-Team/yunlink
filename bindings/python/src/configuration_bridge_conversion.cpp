#include "configuration_bridge_internal.hpp"

#include <stdexcept>

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

namespace yunlink_python_config {

std::string copyView(yunlink_string_view_t view) {
    if (view.size == 0) {
        return {};
    }
    if (view.data == nullptr) {
        throw std::runtime_error("invalid YunLink string view");
    }
    return std::string(view.data, view.size);
}

OwnedValue copyValue(const yunlink_config_value_view_t& view) {
    OwnedValue value;
    value.type = view.type;
    switch (view.type) {
    case YUNLINK_CONFIG_VALUE_BOOL:
        value.data = view.bool_value != 0;
        break;
    case YUNLINK_CONFIG_VALUE_INT64:
        value.data = view.int64_value;
        break;
    case YUNLINK_CONFIG_VALUE_DOUBLE:
        value.data = view.double_value;
        break;
    case YUNLINK_CONFIG_VALUE_STRING:
        value.data = copyView(view.string_value);
        break;
    case YUNLINK_CONFIG_VALUE_STRING_LIST: {
        std::vector<std::string> items;
        const auto* source = checkedArray(view.string_list, view.string_list_count);
        items.reserve(view.string_list_count);
        for (size_t index = 0; index < view.string_list_count; ++index) {
            items.push_back(copyView(source[index]));
        }
        value.data = std::move(items);
        break;
    }
    default:
        throw std::runtime_error("unknown YunLink configuration value type");
    }
    return value;
}

OwnedDescriptor copyDescriptor(const yunlink_config_resource_descriptor_view_t& view) {
    return {copyView(view.id),
            copyView(view.title),
            copyView(view.description),
            view.readable != 0,
            view.writable != 0,
            view.apply_supported != 0};
}

OwnedSnapshot copySnapshot(const yunlink_config_snapshot_view_t& view) {
    OwnedSnapshot snapshot;
    snapshot.resource_id = copyView(view.resource_id);
    snapshot.revision = copyView(view.revision);
    snapshot.applied_revision = copyView(view.applied_revision);
    const auto* values = checkedArray(view.values, view.value_count);
    snapshot.values.reserve(view.value_count);
    for (size_t index = 0; index < view.value_count; ++index) {
        snapshot.values.push_back({copyView(values[index].path), copyValue(values[index].value)});
    }
    return snapshot;
}

OwnedEffects copyEffects(const yunlink_config_effects_view_t& view) {
    OwnedEffects effects;
    effects.requirement = view.requirement;
    effects.reconnect_expected = view.reconnect_expected != 0;
    const auto* components = checkedArray(view.affected_components, view.affected_component_count);
    effects.affected_components.reserve(view.affected_component_count);
    for (size_t index = 0; index < view.affected_component_count; ++index) {
        effects.affected_components.push_back(copyView(components[index]));
    }
    return effects;
}

namespace {

nb::dict valueToPython(const OwnedValue& value) {
    nb::dict output;
    output["type"] = value.type;
    std::visit([&output](const auto& item) { output["value"] = nb::cast(item); }, value.data);
    return output;
}

nb::dict descriptorToPython(const OwnedDescriptor& value) {
    nb::dict output;
    output["id"] = value.id;
    output["title"] = value.title;
    output["description"] = value.description;
    output["readable"] = value.readable;
    output["writable"] = value.writable;
    output["apply_supported"] = value.apply_supported;
    return output;
}

nb::dict snapshotToPython(const OwnedSnapshot& value) {
    nb::dict output;
    output["resource_id"] = value.resource_id;
    output["revision"] = value.revision;
    output["applied_revision"] = value.applied_revision;
    nb::list values;
    for (const auto& item : value.values) {
        nb::dict field;
        field["path"] = item.path;
        field["value"] = valueToPython(item.value);
        values.append(field);
    }
    output["values"] = values;
    return output;
}

nb::dict effectsToPython(const OwnedEffects& value) {
    nb::dict output;
    output["requirement"] = value.requirement;
    output["affected_components"] = nb::cast(value.affected_components);
    output["reconnect_expected"] = value.reconnect_expected;
    return output;
}

}  // namespace

nb::dict responseToPython(const OwnedResponse& value) {
    nb::dict output;
    output["type"] = value.kind;
    output["session_id"] = value.session_id;
    output["message_id"] = value.message_id;
    output["correlation_id"] = value.correlation_id;
    output["status"] = value.status;
    output["message"] = value.message;
    if (value.kind == "configuration_list") {
        nb::list resources;
        for (const auto& resource : value.resources) {
            resources.append(descriptorToPython(resource));
        }
        output["resources"] = resources;
    } else if (value.kind == "configuration_describe") {
        output["resource"] = descriptorToPython(value.resource);
        nb::list fields;
        for (const auto& item : value.fields) {
            nb::dict field;
            field["path"] = item.path;
            field["title"] = item.title;
            field["description"] = item.description;
            field["type"] = item.type;
            field["required"] = item.required;
            field["read_only"] = item.read_only;
            field["sensitive"] = item.sensitive;
            field["minimum"] = item.has_minimum ? nb::cast(item.minimum) : nb::none();
            field["maximum"] = item.has_maximum ? nb::cast(item.maximum) : nb::none();
            field["validation_pattern"] = item.validation_pattern;
            nb::list choices;
            for (const auto& choice : item.choices) {
                nb::dict choice_dict;
                choice_dict["value"] = valueToPython(choice.value);
                choice_dict["label"] = choice.label;
                choices.append(choice_dict);
            }
            field["choices"] = choices;
            fields.append(field);
        }
        output["fields"] = fields;
    } else if (value.kind == "configuration_get") {
        output["snapshot"] = snapshotToPython(value.snapshot);
    } else if (value.kind == "configuration_patch") {
        output["snapshot"] = snapshotToPython(value.snapshot);
        nb::list errors;
        for (const auto& item : value.errors) {
            nb::dict error;
            error["path"] = item.path;
            error["code"] = item.code;
            error["message"] = item.message;
            errors.append(error);
        }
        output["errors"] = errors;
        output["effects"] = effectsToPython(value.effects);
    } else if (value.kind == "configuration_apply") {
        output["applied_revision"] = value.applied_revision;
        output["outcome"] = value.outcome;
        output["effects"] = effectsToPython(value.effects);
    }
    return output;
}

void throwIfError(yunlink_result_t result) {
    if (result != YUNLINK_RESULT_OK) {
        throw std::runtime_error(yunlink_result_name(result));
    }
}

}  // namespace yunlink_python_config
