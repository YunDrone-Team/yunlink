/** @file @brief Borrowed C views backed by one callback-local arena. */

#ifndef YUNLINK_C_ABI_CONFIGURATION_VIEWS_HPP
#define YUNLINK_C_ABI_CONFIGURATION_VIEWS_HPP

#include "../internal.hpp"

#include <deque>

namespace yunlink_c_abi::configuration_views {

inline yunlink_string_view_t view_of(const std::string& value) {
    return {value.data(), value.size()};
}

struct ViewArena {
    std::deque<std::vector<yunlink_string_view_t>> string_lists;
    std::deque<std::vector<double>> double_lists;
    std::deque<std::vector<yunlink_config_choice_view_t>> choice_lists;
    std::vector<yunlink_config_resource_descriptor_view_t> resources;
    std::vector<yunlink_config_field_schema_view_t> fields;
    std::vector<yunlink_config_field_value_view_t> values;
    std::vector<yunlink_config_field_error_view_t> errors;
    std::vector<yunlink_string_view_t> components;

    yunlink_config_value_view_t value(const yunlink::ConfigValue& source) {
        yunlink_config_value_view_t target{};
        target.type = static_cast<uint8_t>(source.type);
        target.bool_value = source.bool_value ? 1 : 0;
        target.int64_value = source.int64_value;
        target.double_value = source.double_value;
        target.string_value = view_of(source.string_value);
        if (source.type == yunlink::ConfigValueType::kStringList) {
            string_lists.emplace_back();
            auto& items = string_lists.back();
            items.reserve(source.string_list_value.size());
            for (const auto& item : source.string_list_value) {
                items.push_back(view_of(item));
            }
            target.string_list = items.data();
            target.string_list_count = items.size();
        } else if (source.type == yunlink::ConfigValueType::kDoubleList) {
            double_lists.push_back(source.double_list_value);
            const auto& items = double_lists.back();
            target.double_list = items.data();
            target.double_list_count = items.size();
        }
        return target;
    }

    yunlink_config_resource_descriptor_view_t
    descriptor(const yunlink::ConfigResourceDescriptor& source) {
        return {view_of(source.id),
                view_of(source.title),
                view_of(source.description),
                static_cast<uint8_t>(source.readable),
                static_cast<uint8_t>(source.writable),
                static_cast<uint8_t>(source.apply_supported)};
    }

    yunlink_config_snapshot_view_t snapshot(const yunlink::ConfigSnapshot& source) {
        values.reserve(source.values.size());
        for (const auto& item : source.values) {
            values.push_back({view_of(item.path), value(item.value)});
        }
        return {view_of(source.resource_id),
                view_of(source.revision),
                view_of(source.applied_revision),
                values.data(),
                values.size()};
    }

    yunlink_config_effects_view_t effects(const yunlink::ConfigEffects& source) {
        components.reserve(source.affected_components.size());
        for (const auto& component : source.affected_components) {
            components.push_back(view_of(component));
        }
        return {static_cast<uint8_t>(source.requirement),
                components.data(),
                components.size(),
                static_cast<uint8_t>(source.reconnect_expected)};
    }

    void describe_fields(const std::vector<yunlink::ConfigFieldSchema>& source) {
        fields.reserve(source.size());
        for (const auto& item : source) {
            choice_lists.emplace_back();
            auto& choices = choice_lists.back();
            choices.reserve(item.choices.size());
            for (const auto& choice : item.choices) {
                choices.push_back({value(choice.value), view_of(choice.label)});
            }
            fields.push_back({view_of(item.path),
                              view_of(item.title),
                              view_of(item.description),
                              static_cast<uint8_t>(item.type),
                              static_cast<uint8_t>(item.required),
                              static_cast<uint8_t>(item.read_only),
                              static_cast<uint8_t>(item.sensitive),
                              static_cast<uint8_t>(item.has_minimum),
                              item.minimum,
                              static_cast<uint8_t>(item.has_maximum),
                              item.maximum,
                              view_of(item.validation_pattern),
                              choices.data(),
                              choices.size()});
        }
    }
};

}  // namespace yunlink_c_abi::configuration_views

#endif  // YUNLINK_C_ABI_CONFIGURATION_VIEWS_HPP
