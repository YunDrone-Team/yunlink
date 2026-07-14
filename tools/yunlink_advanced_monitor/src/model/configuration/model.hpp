#ifndef YUNLINK_ADVANCED_MONITOR_MODEL_CONFIGURATION_MODEL_HPP
#define YUNLINK_ADVANCED_MONITOR_MODEL_CONFIGURATION_MODEL_HPP

#include <string>
#include <unordered_map>
#include <vector>

#include <yunlink/core/semantic/configuration/service_types.hpp>

struct MonitorConfigurationResourceState {
    yunlink::ConfigResourceDescriptor descriptor;
    std::vector<yunlink::ConfigFieldSchema> fields;
    yunlink::ConfigSnapshot snapshot;
    bool schema_pending{false};
    bool snapshot_pending{false};
    bool has_schema{false};
    bool has_snapshot{false};
};

struct MonitorConfigurationState {
    bool supported{false};
    bool list_pending{false};
    std::string last_status;
    std::vector<yunlink::ConfigResourceDescriptor> resources;
    std::unordered_map<std::string, MonitorConfigurationResourceState> resource_states;
    yunlink::ConfigResourcePatchResponse last_patch;
    yunlink::ConfigResourceApplyResponse last_apply;
};

#endif  // YUNLINK_ADVANCED_MONITOR_MODEL_CONFIGURATION_MODEL_HPP
