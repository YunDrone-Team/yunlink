#include "managed_entity_bridge.hpp"

#include <stdexcept>

#include "ffi_helpers.hpp"

namespace nb = nanobind;
using namespace yunlink_python_ffi;

namespace {

std::string copy_view(yunlink_string_view_t view) {
    if (view.size == 0) {
        return {};
    }
    if (view.data == nullptr) {
        throw std::runtime_error("invalid YunLink string view");
    }
    return {view.data, view.size};
}

template <typename T> const T* checked(const T* pointer, size_t count) {
    if (count != 0 && pointer == nullptr) {
        throw std::runtime_error("invalid YunLink array view");
    }
    return pointer;
}

yunlink_target_selector_t target_from_dict(const nb::dict& target) {
    return target_from_python(target);
}

}  // namespace

void ManagedEntityBridge::push(Event event) noexcept {
    try {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.size() >= 256) {
            events_.pop_front();
        }
        events_.push_back(std::move(event));
    } catch (...) {
    }
}

void ManagedEntityBridge::attach(yunlink_runtime_t* runtime) {
    size_t token = 0;
    throw_if_error(yunlink_system_service_subscribe_managed_entity_list_responses(
        runtime, &onDirectory, this, &token));
    tokens_.push_back(token);
    token = 0;
    try {
        throw_if_error(yunlink_system_service_subscribe_managed_entity_directory_changed(
            runtime, &onChanged, this, &token));
        tokens_.push_back(token);
        token = 0;
        throw_if_error(yunlink_system_service_subscribe_managed_entity_attachment_responses(
            runtime, &onAttachment, this, &token));
        tokens_.push_back(token);
    } catch (...) {
        detach(runtime);
        throw;
    }
}

void ManagedEntityBridge::detach(yunlink_runtime_t* runtime) noexcept {
    for (size_t token : tokens_) {
        (void)yunlink_system_service_unsubscribe(runtime, token);
    }
    tokens_.clear();
}

void ManagedEntityBridge::onDirectory(
    void* user_data,
    const yunlink_managed_entity_list_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        Event event;
        event.session_id = response->session_id;
        event.message_id = response->message_id;
        event.correlation_id = response->correlation_id;
        event.success = response->success != 0;
        event.message = copy_view(response->message);
        event.endpoint_uid = copy_view(response->endpoint_uid);
        event.revision = copy_view(response->revision);
        const auto copy_identity = [](const yunlink_managed_entity_identity_view_t& source) {
            Identity identity;
            identity.agent_type = source.agent_type;
            identity.agent_id = source.agent_id;
            identity.role = source.role;
            checked(source.group_ids, source.group_id_count);
            if (source.group_id_count != 0) {
                identity.group_ids.assign(source.group_ids,
                                          source.group_ids + source.group_id_count);
            }
            return identity;
        };
        event.primary_identity = copy_identity(response->primary_identity);
        checked(response->entities, response->entity_count);
        event.entities.reserve(response->entity_count);
        for (size_t index = 0; index < response->entity_count; ++index) {
            const auto& source = response->entities[index];
            Entity entity;
            entity.entity_uid = copy_view(source.entity_uid);
            entity.identity = copy_identity(source.identity);
            entity.display_name = copy_view(source.display_name);
            entity.hardware_id = copy_view(source.hardware_id);
            entity.availability = source.availability;
            checked(source.capabilities, source.capability_count);
            for (size_t capability = 0; capability < source.capability_count; ++capability) {
                entity.capabilities.push_back(copy_view(source.capabilities[capability]));
            }
            event.entities.push_back(std::move(entity));
        }
        static_cast<ManagedEntityBridge*>(user_data)->push(std::move(event));
    } catch (...) {
    }
}

void ManagedEntityBridge::onChanged(
    void* user_data,
    const yunlink_managed_entity_directory_changed_view_t* source) noexcept {
    if (user_data == nullptr || source == nullptr) {
        return;
    }
    try {
        Event event;
        event.kind = EventKind::kDirectoryChanged;
        event.session_id = source->session_id;
        event.message_id = source->message_id;
        event.correlation_id = source->correlation_id;
        event.endpoint_uid = copy_view(source->endpoint_uid);
        event.revision = copy_view(source->revision);
        static_cast<ManagedEntityBridge*>(user_data)->push(std::move(event));
    } catch (...) {
    }
}

void ManagedEntityBridge::onAttachment(
    void* user_data,
    const yunlink_managed_entity_attachment_response_view_t* response) noexcept {
    if (user_data == nullptr || response == nullptr) {
        return;
    }
    try {
        Event event;
        event.kind = EventKind::kAttachment;
        event.session_id = response->session_id;
        event.message_id = response->message_id;
        event.correlation_id = response->correlation_id;
        event.success = response->success != 0;
        event.message = copy_view(response->message);
        event.endpoint_uid = copy_view(response->endpoint_uid);
        event.revision = copy_view(response->directory_revision);
        checked(response->results, response->result_count);
        event.attachment_results.reserve(response->result_count);
        for (size_t index = 0; index < response->result_count; ++index) {
            const auto& source = response->results[index];
            event.attachment_results.push_back(
                {copy_view(source.entity_uid), source.accepted != 0, copy_view(source.message)});
        }
        checked(response->attached_entity_uids, response->attached_entity_count);
        event.attached_entity_uids.reserve(response->attached_entity_count);
        for (size_t index = 0; index < response->attached_entity_count; ++index) {
            event.attached_entity_uids.push_back(copy_view(response->attached_entity_uids[index]));
        }
        static_cast<ManagedEntityBridge*>(user_data)->push(std::move(event));
    } catch (...) {
    }
}

nb::object ManagedEntityBridge::poll() {
    Event event;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (events_.empty()) {
            return nb::none();
        }
        event = std::move(events_.front());
        events_.pop_front();
    }
    const auto identity_to_python = [](const Identity& identity) {
        nb::dict out;
        out["agent_type"] = identity.agent_type;
        out["agent_id"] = identity.agent_id;
        out["role"] = identity.role;
        nb::list groups;
        for (uint32_t group : identity.group_ids) {
            groups.append(group);
        }
        out["group_ids"] = std::move(groups);
        return out;
    };
    nb::dict out;
    out["type"] = event.kind == EventKind::kDirectoryChanged ? "managed_entity_directory_changed"
                  : event.kind == EventKind::kAttachment     ? "managed_entity_attachment"
                                                             : "managed_entity_directory";
    out["session_id"] = event.session_id;
    out["message_id"] = event.message_id;
    out["correlation_id"] = event.correlation_id;
    out["endpoint_uid"] = event.endpoint_uid;
    out["revision"] = event.revision;
    if (event.kind == EventKind::kDirectory) {
        out["success"] = event.success;
        out["message"] = event.message;
        out["primary_identity"] = identity_to_python(event.primary_identity);
        nb::list entities;
        for (const auto& entity : event.entities) {
            nb::dict value;
            value["entity_uid"] = entity.entity_uid;
            value["identity"] = identity_to_python(entity.identity);
            value["display_name"] = entity.display_name;
            value["hardware_id"] = entity.hardware_id;
            value["availability"] = entity.availability;
            nb::list capabilities;
            for (const auto& capability : entity.capabilities) {
                capabilities.append(capability);
            }
            value["capabilities"] = std::move(capabilities);
            entities.append(std::move(value));
        }
        out["entities"] = std::move(entities);
    } else if (event.kind == EventKind::kAttachment) {
        out["success"] = event.success;
        out["message"] = event.message;
        out["directory_revision"] = event.revision;
        nb::list results;
        for (const auto& result : event.attachment_results) {
            nb::dict value;
            value["entity_uid"] = result.entity_uid;
            value["accepted"] = result.accepted;
            value["message"] = result.message;
            results.append(std::move(value));
        }
        out["results"] = std::move(results);
        nb::list attached;
        for (const auto& entity_uid : event.attached_entity_uids) {
            attached.append(entity_uid);
        }
        out["attached_entity_uids"] = std::move(attached);
    }
    return nb::cast(out);
}

nb::dict ManagedEntityBridge::request_attachment(yunlink_runtime_t* runtime,
                                                 const std::string& peer_id,
                                                 uint64_t session_id,
                                                 const nb::dict& target,
                                                 const std::string& endpoint_uid,
                                                 const std::string& directory_revision,
                                                 const std::string& action,
                                                 const nb::list& entity_uids) {
    const auto peer = peer_from_python(peer_id);
    const yunlink_session_t session{session_id};
    const auto native_target = target_from_dict(target);
    const uint8_t native_action = action == "attach" ? 1U : action == "detach" ? 2U : 0U;
    if (endpoint_uid.empty() || directory_revision.empty() || native_action == 0U ||
        entity_uids.size() == 0U) {
        throw std::invalid_argument("invalid managed entity attachment request");
    }
    std::vector<std::string> owned_entity_uids;
    owned_entity_uids.reserve(entity_uids.size());
    std::vector<yunlink_string_view_t> views;
    views.reserve(entity_uids.size());
    for (nb::handle value : entity_uids) {
        const std::string entity_uid = nb::cast<std::string>(value);
        if (entity_uid.empty()) {
            throw std::invalid_argument("managed entity uid must not be empty");
        }
        owned_entity_uids.push_back(entity_uid);
    }
    for (const auto& entity_uid : owned_entity_uids) {
        views.push_back({entity_uid.data(), entity_uid.size()});
    }
    const yunlink_string_view_t endpoint_view{endpoint_uid.data(), endpoint_uid.size()};
    const yunlink_string_view_t revision_view{directory_revision.data(), directory_revision.size()};
    yunlink_command_handle_t handle{};
    throw_if_error(yunlink_system_service_request_managed_entity_attachment(runtime,
                                                                            &peer,
                                                                            &session,
                                                                            &native_target,
                                                                            endpoint_view,
                                                                            revision_view,
                                                                            native_action,
                                                                            views.data(),
                                                                            views.size(),
                                                                            &handle));
    nb::dict out;
    out["session_id"] = handle.session_id;
    out["message_id"] = handle.message_id;
    out["correlation_id"] = handle.correlation_id;
    return out;
}

nb::dict ManagedEntityBridge::request(yunlink_runtime_t* runtime,
                                      const std::string& peer_id,
                                      uint64_t session_id,
                                      const nb::dict& target) {
    const auto peer = peer_from_python(peer_id);
    const yunlink_session_t session{session_id};
    const auto native_target = target_from_dict(target);
    yunlink_command_handle_t handle{};
    throw_if_error(yunlink_system_service_request_managed_entity_list(
        runtime, &peer, &session, &native_target, &handle));
    nb::dict out;
    out["session_id"] = handle.session_id;
    out["message_id"] = handle.message_id;
    out["correlation_id"] = handle.correlation_id;
    return out;
}
