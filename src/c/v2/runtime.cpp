#include "yunlink/c/yunlink_v2.h"

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "yunlink/runtime/runtime_v2.hpp"

struct yunlink_v2_runtime {
    yunlink::v2::Runtime runtime;
};

namespace {

std::string copy(yunlink_v2_string_view_t value) {
    if (value.data == nullptr || value.len == 0) {
        return {};
    }
    return std::string(value.data, value.len);
}

yunlink_v2_string_view_t view(const std::string& value) {
    return {value.data(), value.size()};
}

yunlink_v2_bytes_view_t view(const yunlink::v2::Bytes& value) {
    return {value.data(), value.size()};
}

uint16_t result(yunlink::v2::ErrorCode code) {
    return static_cast<uint16_t>(code);
}

bool copy_profiles(const yunlink_v2_profile_view_t* values,
                   size_t count,
                   std::vector<yunlink::v2::ProfileDescriptor>* out) {
    if ((values == nullptr) != (count == 0) || out == nullptr) {
        return false;
    }
    out->reserve(count);
    for (size_t i = 0; i < count; ++i) {
        yunlink::v2::ProfileDescriptor profile;
        profile.profile_id = copy(values[i].profile_id);
        profile.major = values[i].major;
        profile.minor = values[i].minor;
        profile.schema_digest = copy(values[i].schema_digest);
        out->push_back(std::move(profile));
    }
    return true;
}

bool copy_target(yunlink_v2_target_view_t value, yunlink::v2::TargetSelector* out) {
    if (out == nullptr || value.scope < 1 || value.scope > 4 ||
        ((value.uids == nullptr) != (value.uid_count == 0))) {
        return false;
    }
    out->scope = static_cast<yunlink::v2::TargetScope>(value.scope);
    out->uids.reserve(value.uid_count);
    for (size_t i = 0; i < value.uid_count; ++i) {
        out->uids.push_back(copy(value.uids[i]));
    }
    return true;
}

template <size_t Size> void copy_fixed(char (&target)[Size], const std::string& value) {
    const size_t count = std::min(value.size(), Size - 1);
    std::memcpy(target, value.data(), count);
    target[count] = '\0';
}

}  // namespace

extern "C" {

uint32_t yunlink_v2_abi_version(void) {
    return YUNLINK_V2_ABI_VERSION;
}

yunlink_v2_runtime_t* yunlink_v2_runtime_create(void) {
    return new yunlink_v2_runtime_t();
}

void yunlink_v2_runtime_destroy(yunlink_v2_runtime_t* runtime) {
    delete runtime;
}

uint16_t yunlink_v2_runtime_start(yunlink_v2_runtime_t* runtime,
                                  const yunlink_v2_runtime_config_t* config) {
    if (runtime == nullptr || config == nullptr ||
        config->struct_size < sizeof(yunlink_v2_runtime_config_t)) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    yunlink::v2::RuntimeConfig native;
    native.endpoint_uid = copy(config->endpoint_uid);
    native.display_name = copy(config->display_name);
    native.shared_secret = copy(config->shared_secret);
    native.tcp_listen_port = config->tcp_listen_port;
    if (!copy_profiles(config->profiles, config->profile_count, &native.profiles) ||
        !copy_profiles(
            config->required_profiles, config->required_profile_count, &native.required_profiles)) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    return result(runtime->runtime.start(native));
}

void yunlink_v2_runtime_stop(yunlink_v2_runtime_t* runtime) {
    if (runtime != nullptr) {
        runtime->runtime.stop();
    }
}

uint16_t yunlink_v2_runtime_connect(yunlink_v2_runtime_t* runtime,
                                    yunlink_v2_string_view_t ip,
                                    uint16_t port,
                                    yunlink_v2_peer_t* out_peer) {
    if (runtime == nullptr) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    yunlink::v2::Peer peer;
    const auto code = runtime->runtime.connect_peer(copy(ip), port, &peer);
    if (code == yunlink::v2::ErrorCode::kOk && out_peer != nullptr) {
        *out_peer = {};
        copy_fixed(out_peer->id, peer.id);
        copy_fixed(out_peer->ip, peer.ip);
        out_peer->port = peer.port;
    }
    return result(code);
}

void yunlink_v2_runtime_close_peer(yunlink_v2_runtime_t* runtime,
                                   yunlink_v2_string_view_t peer_id) {
    if (runtime != nullptr) {
        runtime->runtime.close_peer(copy(peer_id));
    }
}

uint64_t yunlink_v2_runtime_open_session(yunlink_v2_runtime_t* runtime,
                                         yunlink_v2_string_view_t peer_id) {
    return runtime == nullptr ? 0 : runtime->runtime.open_session(copy(peer_id));
}

uint16_t yunlink_v2_runtime_session_endpoint_uid(const yunlink_v2_runtime_t* runtime,
                                                 yunlink_v2_string_view_t peer_id,
                                                 uint64_t session_id,
                                                 char* out_uid,
                                                 size_t out_uid_capacity) {
    if (runtime == nullptr || out_uid == nullptr || out_uid_capacity == 0) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    yunlink::v2::SessionInfo session;
    if (!runtime->runtime.session(copy(peer_id), session_id, &session) ||
        session.remote_endpoint_uid.empty()) {
        return result(yunlink::v2::ErrorCode::kNotFound);
    }
    if (session.remote_endpoint_uid.size() + 1 > out_uid_capacity) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    std::memcpy(out_uid, session.remote_endpoint_uid.data(), session.remote_endpoint_uid.size());
    out_uid[session.remote_endpoint_uid.size()] = '\0';
    return result(yunlink::v2::ErrorCode::kOk);
}

uint16_t yunlink_v2_runtime_publish(yunlink_v2_runtime_t* runtime,
                                    yunlink_v2_string_view_t peer_id,
                                    uint64_t session_id,
                                    uint8_t family,
                                    uint8_t operation,
                                    yunlink_v2_target_view_t target,
                                    yunlink_v2_type_ref_view_t type_ref,
                                    yunlink_v2_bytes_view_t payload,
                                    uint64_t correlation_id,
                                    uint32_t ttl_ms,
                                    uint8_t qos_class,
                                    yunlink_v2_string_view_t source_entity_uid,
                                    yunlink_v2_message_handle_t* out_handle) {
    if (runtime == nullptr || (payload.data == nullptr && payload.len != 0) || family < 1 ||
        family > 9 || qos_class < 1 || qos_class > 4) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    yunlink::v2::TargetSelector native_target;
    if (!copy_target(target, &native_target)) {
        return result(yunlink::v2::ErrorCode::kInvalidArgument);
    }
    const yunlink::v2::TypeRef native_type{
        copy(type_ref.profile_id), type_ref.major, type_ref.minor, copy(type_ref.type_name)};
    yunlink::v2::Bytes native_payload;
    if (payload.len > 0) {
        native_payload.assign(payload.data, payload.data + payload.len);
    }
    yunlink::v2::MessageHandle handle;
    const auto code = runtime->runtime.publish(copy(peer_id),
                                               session_id,
                                               static_cast<yunlink::v2::MessageFamily>(family),
                                               operation,
                                               native_target,
                                               native_type,
                                               native_payload,
                                               &handle,
                                               correlation_id,
                                               ttl_ms,
                                               static_cast<yunlink::v2::QosClass>(qos_class),
                                               copy(source_entity_uid));
    if (code == yunlink::v2::ErrorCode::kOk && out_handle != nullptr) {
        *out_handle = {handle.session_id, handle.message_id, handle.correlation_id};
    }
    return result(code);
}

uint64_t yunlink_v2_runtime_subscribe(yunlink_v2_runtime_t* runtime,
                                      yunlink_v2_event_callback_t callback,
                                      void* user_data) {
    if (runtime == nullptr || callback == nullptr) {
        return 0;
    }
    return runtime->runtime.subscribe(
        [callback, user_data](const yunlink::v2::RuntimeEvent& value) {
            std::vector<yunlink_v2_string_view_t> target_uids;
            target_uids.reserve(value.envelope.target.uids.size());
            for (const auto& uid : value.envelope.target.uids) {
                target_uids.push_back(view(uid));
            }
            yunlink_v2_event_t event{};
            event.kind = static_cast<uint8_t>(value.kind);
            event.peer_id = view(value.peer.id);
            event.link_up = value.link_up ? 1 : 0;
            event.error_code = static_cast<uint16_t>(value.error);
            event.message = view(value.message);
            event.session_state = static_cast<uint8_t>(value.session.state);
            event.session_authenticated = value.session.authenticated ? 1 : 0;
            event.session_id = value.kind == yunlink::v2::RuntimeEventKind::kEnvelope
                                   ? value.envelope.session_id
                                   : value.session.session_id;
            event.family = static_cast<uint8_t>(value.envelope.family);
            event.operation = value.envelope.operation;
            event.qos_class = static_cast<uint8_t>(value.envelope.qos_class);
            event.message_id = value.envelope.message_id;
            event.correlation_id = value.envelope.correlation_id;
            event.created_at_ms = value.envelope.created_at_ms;
            event.ttl_ms = value.envelope.ttl_ms;
            event.source_endpoint_uid = view(value.envelope.source.endpoint_uid);
            event.source_entity_uid = view(value.envelope.source.entity_uid);
            event.target = {static_cast<uint8_t>(value.envelope.target.scope),
                            target_uids.data(),
                            target_uids.size()};
            event.type_ref = {view(value.envelope.type.profile_id),
                              value.envelope.type.major,
                              value.envelope.type.minor,
                              view(value.envelope.type.type_name)};
            event.payload = view(value.envelope.payload);
            callback(&event, user_data);
        });
}

void yunlink_v2_runtime_unsubscribe(yunlink_v2_runtime_t* runtime, uint64_t token) {
    if (runtime != nullptr) {
        runtime->runtime.unsubscribe(static_cast<size_t>(token));
    }
}

uint8_t yunlink_v2_runtime_session_has_profile(const yunlink_v2_runtime_t* runtime,
                                               yunlink_v2_string_view_t peer_id,
                                               uint64_t session_id,
                                               yunlink_v2_string_view_t profile_id,
                                               uint16_t major) {
    if (runtime == nullptr) {
        return 0;
    }
    yunlink::v2::SessionInfo session;
    return runtime->runtime.session(copy(peer_id), session_id, &session) &&
                   session.has_profile(copy(profile_id), major)
               ? 1
               : 0;
}

uint8_t yunlink_v2_runtime_session_supports_profile(const yunlink_v2_runtime_t* runtime,
                                                    yunlink_v2_string_view_t peer_id,
                                                    uint64_t session_id,
                                                    yunlink_v2_string_view_t profile_id,
                                                    uint16_t major,
                                                    uint16_t minimum_minor) {
    if (runtime == nullptr) {
        return 0;
    }
    return runtime->runtime.session_supports_profile(
               copy(peer_id), session_id, copy(profile_id), major, minimum_minor)
               ? 1
               : 0;
}

}  // extern "C"
