#include <string>

#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "yunlink/c/yunlink_c.h"
#include "configuration_bridge.hpp"
#include "ffi_helpers.hpp"
#include "managed_entity_bridge.hpp"

namespace nb = nanobind;
using namespace yunlink_python_ffi;

class RuntimeCore {
  public:
    RuntimeCore() {
        throw_if_error(yunlink_runtime_create(&runtime_));
        try {
            configuration_.attach(runtime_);
            managed_entities_.attach(runtime_);
        } catch (...) {
            managed_entities_.detach(runtime_);
            configuration_.detach(runtime_);
            yunlink_runtime_destroy(runtime_);
            runtime_ = nullptr;
            throw;
        }
    }

    ~RuntimeCore() {
        if (runtime_ != nullptr) {
            managed_entities_.detach(runtime_);
            configuration_.detach(runtime_);
            (void)yunlink_runtime_stop(runtime_);
            yunlink_runtime_destroy(runtime_);
            runtime_ = nullptr;
        }
    }

    void start(const nb::dict& config) {
        yunlink_runtime_config_t native{};
        native.struct_size = sizeof(native);
        native.udp_bind_port = nb::cast<uint16_t>(config["udp_bind_port"]);
        native.udp_target_port = nb::cast<uint16_t>(config["udp_target_port"]);
        native.tcp_listen_port = nb::cast<uint16_t>(config["tcp_listen_port"]);
        native.connect_timeout_ms = 5000;
        native.io_poll_interval_ms = 5;
        native.max_buffer_bytes_per_peer = static_cast<size_t>(1) << 20;
        native.self_identity.agent_type = nb::cast<uint8_t>(config["agent_type"]);
        native.self_identity.agent_id = nb::cast<uint32_t>(config["agent_id"]);
        native.self_identity.role = nb::cast<uint8_t>(config["role"]);
        if (config.contains("capability_flags")) {
            native.capability_flags = nb::cast<uint32_t>(config["capability_flags"]);
        }
        if (config.contains("required_peer_capability_flags")) {
            native.required_peer_capability_flags =
                nb::cast<uint32_t>(config["required_peer_capability_flags"]);
        }
        std::vector<yunlink_identity_t> managed_identities;
        if (config.contains("managed_identities")) {
            const auto values = nb::cast<nb::list>(config["managed_identities"]);
            managed_identities.reserve(values.size());
            for (const nb::handle value : values) {
                const auto identity = nb::cast<nb::dict>(value);
                managed_identities.push_back({nb::cast<uint8_t>(identity["agent_type"]),
                                              nb::cast<uint32_t>(identity["agent_id"]),
                                              nb::cast<uint8_t>(identity["role"])});
            }
        }
        native.managed_identities = managed_identities.data();
        native.managed_identity_count = managed_identities.size();
        copy_string(native.shared_secret,
                    sizeof(native.shared_secret),
                    nb::cast<std::string>(config["shared_secret"]));
        copy_string(native.multicast_group,
                    sizeof(native.multicast_group),
                    nb::cast<std::string>(config["multicast_group"]));
        native.qos_profile = 2;
        native.qos_reliable_ordered_transport = 1;
        native.qos_reliable_latest_transport = 1;
        native.qos_best_effort_transport = 2;
        native.qos_bulk_transport = 2;
        native.qos_udp_fallback_to_tcp = 1;
        native.security_key_epoch = 1;
        native.security_tags_enabled = 1;
        throw_if_error(yunlink_runtime_start(runtime_, &native));
    }

    void stop() {
        throw_if_error(yunlink_runtime_stop(runtime_));
    }

    std::string connect(const std::string& ip, uint16_t port) {
        yunlink_peer_t peer{};
        throw_if_error(yunlink_peer_connect(runtime_, ip.c_str(), port, &peer));
        return std::string(peer.id);
    }

    uint64_t open_session(const std::string& peer_id, const std::string& node_name) {
        auto peer = peer_from_python(peer_id);
        yunlink_session_t session{};
        throw_if_error(yunlink_session_open(runtime_, &peer, node_name.c_str(), &session));
        return session.session_id;
    }

    void request_authority(const std::string& peer_id,
                           uint64_t session_id,
                           const nb::dict& target,
                           uint8_t source,
                           uint32_t lease_ttl_ms,
                           bool allow_preempt) {
        auto peer = peer_from_python(peer_id);
        const auto native_target = target_from_python(target);
        yunlink_session_t session{session_id};
        throw_if_error(yunlink_authority_request(runtime_,
                                                 &peer,
                                                 &session,
                                                 &native_target,
                                                 source,
                                                 lease_ttl_ms,
                                                 allow_preempt ? 1 : 0));
    }

    void
    release_authority(const std::string& peer_id, uint64_t session_id, const nb::dict& target) {
        auto peer = peer_from_python(peer_id);
        const auto native_target = target_from_python(target);
        yunlink_session_t session{session_id};
        throw_if_error(yunlink_authority_release(runtime_, &peer, &session, &native_target));
    }

    void renew_authority(const std::string& peer_id,
                         uint64_t session_id,
                         const nb::dict& target,
                         uint8_t source,
                         uint32_t lease_ttl_ms) {
        auto peer = peer_from_python(peer_id);
        const auto native_target = target_from_python(target);
        yunlink_session_t session{session_id};
        throw_if_error(yunlink_authority_renew(
            runtime_, &peer, &session, &native_target, source, lease_ttl_ms));
    }

    nb::object current_authority() {
        yunlink_authority_lease_t lease{};
        const auto result = yunlink_authority_current(runtime_, &lease);
        if (result == YUNLINK_RESULT_NOT_FOUND) {
            return nb::none();
        }
        throw_if_error(result);

        nb::dict out;
        out["state"] = lease.state;
        out["session_id"] = lease.session_id;
        out["peer_id"] = std::string(lease.peer.id);
        return nb::cast(out);
    }

    nb::dict publish_goto(const std::string& peer_id,
                          uint64_t session_id,
                          const nb::dict& target,
                          const nb::dict& payload) {
        auto peer = peer_from_python(peer_id);
        const auto native_target = target_from_python(target);
        const yunlink_session_t session{session_id};
        yunlink_goto_command_t native{};
        native.x_m = nb::cast<float>(payload["x_m"]);
        native.y_m = nb::cast<float>(payload["y_m"]);
        native.z_m = nb::cast<float>(payload["z_m"]);
        native.yaw_rad = nb::cast<float>(payload["yaw_rad"]);
        yunlink_command_handle_t handle{};
        throw_if_error(yunlink_command_publish_goto(
            runtime_, &peer, &session, &native_target, &native, &handle));

        nb::dict out;
        out["session_id"] = handle.session_id;
        out["message_id"] = handle.message_id;
        out["correlation_id"] = handle.correlation_id;
        return out;
    }

    void publish_vehicle_core_state(const std::string& peer_id,
                                    const nb::dict& target,
                                    const nb::dict& payload,
                                    uint64_t session_id) {
        auto peer = peer_from_python(peer_id);
        const auto native_target = target_from_python(target);
        const auto native_state = state_from_python(payload);
        throw_if_error(yunlink_publish_vehicle_core_state(
            runtime_, &peer, &native_target, &native_state, session_id));
    }

    nb::object poll_event() {
        yunlink_runtime_event_t event{};
        throw_if_error(yunlink_runtime_poll_event(runtime_, &event));
        if (event.type == YUNLINK_RUNTIME_EVENT_NONE) {
            return nb::none();
        }

        nb::dict out;
        if (event.type == YUNLINK_RUNTIME_EVENT_COMMAND_RESULT) {
            out["type"] = "command_result";
            out["session_id"] = event.data.command_result.session_id;
            out["message_id"] = event.data.command_result.message_id;
            out["correlation_id"] = event.data.command_result.correlation_id;
            out["command_kind"] = event.data.command_result.command_kind;
            out["phase"] = event.data.command_result.phase;
            out["result_code"] = event.data.command_result.result_code;
            out["progress_percent"] = event.data.command_result.progress_percent;
            out["detail"] = event.data.command_result.detail;
            return nb::cast(out);
        }
        if (event.type == YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE) {
            out["type"] = "vehicle_core_state";
            out["session_id"] = event.data.vehicle_core_state.session_id;
            out["message_id"] = event.data.vehicle_core_state.message_id;
            out["correlation_id"] = event.data.vehicle_core_state.correlation_id;
            out["armed"] = event.data.vehicle_core_state.armed != 0;
            out["battery_percent"] = event.data.vehicle_core_state.battery_percent;
            return nb::cast(out);
        }
        if (event.type == YUNLINK_RUNTIME_EVENT_LINK) {
            out["type"] = "link";
            out["peer_id"] = std::string(event.data.link.peer_id);
            out["is_up"] = event.data.link.is_up != 0;
            return nb::cast(out);
        }
        if (event.type == YUNLINK_RUNTIME_EVENT_ERROR) {
            out["type"] = "error";
            out["code"] = event.data.error.code;
            out["message"] = std::string(event.data.error.message);
            return nb::cast(out);
        }
        return nb::none();
    }

    nb::dict
    configuration_list(const std::string& peer_id, uint64_t session_id, const nb::dict& target) {
        return configurationList(runtime_, peer_id, session_id, target);
    }

    nb::dict configuration_describe(const std::string& peer_id,
                                    uint64_t session_id,
                                    const nb::dict& target,
                                    const std::string& resource_id) {
        return configurationDescribe(runtime_, peer_id, session_id, target, resource_id);
    }

    nb::dict configuration_get(const std::string& peer_id,
                               uint64_t session_id,
                               const nb::dict& target,
                               const std::string& resource_id) {
        return configurationGet(runtime_, peer_id, session_id, target, resource_id);
    }

    nb::dict configuration_patch(const std::string& peer_id,
                                 uint64_t session_id,
                                 const nb::dict& target,
                                 const std::string& resource_id,
                                 const std::string& expected_revision,
                                 const nb::list& updates,
                                 bool validate_only) {
        return configurationPatch(runtime_,
                                  peer_id,
                                  session_id,
                                  target,
                                  resource_id,
                                  expected_revision,
                                  updates,
                                  validate_only);
    }

    nb::dict configuration_apply(const std::string& peer_id,
                                 uint64_t session_id,
                                 const nb::dict& target,
                                 const std::string& resource_id,
                                 const std::string& expected_revision) {
        return configurationApply(
            runtime_, peer_id, session_id, target, resource_id, expected_revision);
    }

    nb::object poll_configuration_response() {
        return configuration_.poll();
    }

    nb::dict request_managed_entity_list(const std::string& peer_id,
                                         uint64_t session_id,
                                         const nb::dict& target) {
        return managed_entities_.request(runtime_, peer_id, session_id, target);
    }

    nb::dict request_managed_entity_attachment(const std::string& peer_id,
                                               uint64_t session_id,
                                               const nb::dict& target,
                                               const std::string& endpoint_uid,
                                               const std::string& directory_revision,
                                               const std::string& action,
                                               const nb::list& entity_uids) {
        return managed_entities_.request_attachment(runtime_,
                                                    peer_id,
                                                    session_id,
                                                    target,
                                                    endpoint_uid,
                                                    directory_revision,
                                                    action,
                                                    entity_uids);
    }

    nb::object poll_managed_entity_event() {
        return managed_entities_.poll();
    }

  private:
    yunlink_runtime_t* runtime_ = nullptr;
    ConfigurationBridge configuration_;
    ManagedEntityBridge managed_entities_;
};

NB_MODULE(_yunlink_native, m) {
    m.def("abi_version", &yunlink_ffi_abi_version);

    nb::class_<RuntimeCore>(m, "RuntimeCore")
        .def(nb::init<>())
        .def("start", &RuntimeCore::start)
        .def("stop", &RuntimeCore::stop)
        .def("connect", &RuntimeCore::connect)
        .def("open_session", &RuntimeCore::open_session)
        .def("request_authority", &RuntimeCore::request_authority)
        .def("renew_authority", &RuntimeCore::renew_authority)
        .def("release_authority", &RuntimeCore::release_authority)
        .def("current_authority", &RuntimeCore::current_authority)
        .def("publish_goto", &RuntimeCore::publish_goto)
        .def("publish_vehicle_core_state", &RuntimeCore::publish_vehicle_core_state)
        .def("poll_event", &RuntimeCore::poll_event)
        .def("configuration_list", &RuntimeCore::configuration_list)
        .def("configuration_describe", &RuntimeCore::configuration_describe)
        .def("configuration_get", &RuntimeCore::configuration_get)
        .def("configuration_patch", &RuntimeCore::configuration_patch)
        .def("configuration_apply", &RuntimeCore::configuration_apply)
        .def("poll_configuration_response", &RuntimeCore::poll_configuration_response)
        .def("request_managed_entity_list", &RuntimeCore::request_managed_entity_list)
        .def("request_managed_entity_attachment", &RuntimeCore::request_managed_entity_attachment)
        .def("poll_managed_entity_event", &RuntimeCore::poll_managed_entity_event);
}
