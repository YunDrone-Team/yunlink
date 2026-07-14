#include "configuration_bridge.hpp"

#include <algorithm>
#include <cstring>
#include <deque>
#include <stdexcept>

#include <nanobind/stl/string.h>
#include <nanobind/stl/vector.h>

namespace nb = nanobind;

namespace {

void throwIfError(yunlink_result_t result) {
    if (result != YUNLINK_RESULT_OK) {
        throw std::runtime_error(yunlink_result_name(result));
    }
}

yunlink_peer_t makePeer(const std::string& peer_id) {
    yunlink_peer_t peer{};
    const size_t count = std::min(peer_id.size(), sizeof(peer.id) - 1);
    std::memcpy(peer.id, peer_id.data(), count);
    return peer;
}

yunlink_target_selector_t makeTarget(const nb::dict& target) {
    yunlink_target_selector_t output{};
    output.struct_size = sizeof(output);
    output.scope = nb::cast<uint8_t>(target["scope"]);
    output.target_type = nb::cast<uint8_t>(target["target_type"]);
    output.entity_id = nb::cast<uint32_t>(target["entity_id"]);
    output.group_id = nb::cast<uint32_t>(target["group_id"]);
    return output;
}

yunlink_string_view_t makeView(const std::string& value) {
    return {value.data(), value.size()};
}

nb::dict handleToPython(const yunlink_configuration_handle_t& handle) {
    nb::dict output;
    output["message_id"] = handle.message_id;
    output["session_id"] = handle.session_id;
    output["created_at_ms"] = handle.created_at_ms;
    output["ttl_ms"] = handle.ttl_ms;
    return output;
}

struct PatchArena {
    std::deque<std::string> strings;
    std::deque<std::vector<yunlink_string_view_t>> string_lists;
    std::vector<yunlink_config_field_value_view_t> updates;

    yunlink_string_view_t keep(std::string value) {
        strings.push_back(std::move(value));
        return makeView(strings.back());
    }

    void add(const nb::dict& update) {
        const nb::dict value = nb::cast<nb::dict>(update["value"]);
        yunlink_config_value_view_t native{};
        native.type = nb::cast<uint8_t>(value["type"]);
        switch (native.type) {
        case YUNLINK_CONFIG_VALUE_BOOL:
            native.bool_value = nb::cast<bool>(value["value"]) ? 1 : 0;
            break;
        case YUNLINK_CONFIG_VALUE_INT64:
            native.int64_value = nb::cast<int64_t>(value["value"]);
            break;
        case YUNLINK_CONFIG_VALUE_DOUBLE:
            native.double_value = nb::cast<double>(value["value"]);
            break;
        case YUNLINK_CONFIG_VALUE_STRING:
            native.string_value = keep(nb::cast<std::string>(value["value"]));
            break;
        case YUNLINK_CONFIG_VALUE_STRING_LIST: {
            string_lists.emplace_back();
            auto& views = string_lists.back();
            for (const auto& item : nb::cast<std::vector<std::string>>(value["value"])) {
                views.push_back(keep(item));
            }
            native.string_list = views.data();
            native.string_list_count = views.size();
            break;
        }
        default:
            throw std::invalid_argument("unknown configuration value type");
        }
        updates.push_back({keep(nb::cast<std::string>(update["path"])), native});
    }
};

template <typename Publish>
nb::dict publishSimple(yunlink_runtime_t* runtime,
                       const std::string& peer_id,
                       uint64_t session_id,
                       const nb::dict& target,
                       Publish publish) {
    auto peer = makePeer(peer_id);
    const yunlink_session_t session{session_id};
    const auto native_target = makeTarget(target);
    yunlink_configuration_handle_t handle{};
    throwIfError(publish(&peer, &session, &native_target, &handle));
    return handleToPython(handle);
}

}  // namespace

nb::dict configurationList(yunlink_runtime_t* runtime,
                           const std::string& peer_id,
                           uint64_t session_id,
                           const nb::dict& target) {
    return publishSimple(
        runtime, peer_id, session_id, target, [&](auto peer, auto session, auto t, auto handle) {
            return yunlink_configuration_publish_resource_list_request(
                runtime, peer, session, t, handle);
        });
}

nb::dict configurationDescribe(yunlink_runtime_t* runtime,
                               const std::string& peer_id,
                               uint64_t session_id,
                               const nb::dict& target,
                               const std::string& resource_id) {
    return publishSimple(
        runtime, peer_id, session_id, target, [&](auto peer, auto session, auto t, auto handle) {
            return yunlink_configuration_publish_resource_describe_request(
                runtime, peer, session, t, makeView(resource_id), handle);
        });
}

nb::dict configurationGet(yunlink_runtime_t* runtime,
                          const std::string& peer_id,
                          uint64_t session_id,
                          const nb::dict& target,
                          const std::string& resource_id) {
    return publishSimple(
        runtime, peer_id, session_id, target, [&](auto peer, auto session, auto t, auto handle) {
            return yunlink_configuration_publish_resource_get_request(
                runtime, peer, session, t, makeView(resource_id), handle);
        });
}

nb::dict configurationPatch(yunlink_runtime_t* runtime,
                            const std::string& peer_id,
                            uint64_t session_id,
                            const nb::dict& target,
                            const std::string& resource_id,
                            const std::string& expected_revision,
                            const nb::list& updates,
                            bool validate_only) {
    PatchArena arena;
    for (nb::handle item : updates) {
        arena.add(nb::cast<nb::dict>(item));
    }
    return publishSimple(
        runtime, peer_id, session_id, target, [&](auto peer, auto session, auto t, auto handle) {
            return yunlink_configuration_publish_resource_patch_request(runtime,
                                                                        peer,
                                                                        session,
                                                                        t,
                                                                        makeView(resource_id),
                                                                        makeView(expected_revision),
                                                                        arena.updates.data(),
                                                                        arena.updates.size(),
                                                                        validate_only ? 1 : 0,
                                                                        handle);
        });
}

nb::dict configurationApply(yunlink_runtime_t* runtime,
                            const std::string& peer_id,
                            uint64_t session_id,
                            const nb::dict& target,
                            const std::string& resource_id,
                            const std::string& expected_revision) {
    return publishSimple(
        runtime, peer_id, session_id, target, [&](auto peer, auto session, auto t, auto handle) {
            return yunlink_configuration_publish_resource_apply_request(runtime,
                                                                        peer,
                                                                        session,
                                                                        t,
                                                                        makeView(resource_id),
                                                                        makeView(expected_revision),
                                                                        handle);
        });
}
