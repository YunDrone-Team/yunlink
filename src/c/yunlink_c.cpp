/**
 * @file src/c/yunlink_c.cpp
 * @brief yunlink bindings-oriented C ABI implementation.
 */

#include "yunlink/c/yunlink_c.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>

#include "yunlink/runtime/runtime.hpp"

struct yunlink_runtime {
    yunlink::Runtime runtime;
    std::mutex mu;
    std::deque<yunlink_runtime_event_t> queue;
    size_t tok_error = 0;
    size_t tok_link = 0;
    size_t tok_vehicle_core = 0;
    size_t tok_vehicle_event = 0;
    size_t tok_command_result = 0;
    bool started = false;
};

namespace {

void safe_copy(char* dst, size_t cap, const std::string& src) {
    if (!dst || cap == 0) {
        return;
    }
    const size_t n = std::min(cap - 1, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

yunlink_result_t to_result(yunlink::ErrorCode code) {
    switch (code) {
    case yunlink::ErrorCode::kOk:
        return YUNLINK_RESULT_OK;
    case yunlink::ErrorCode::kInvalidArgument:
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    case yunlink::ErrorCode::kSocketError:
        return YUNLINK_RESULT_SOCKET_ERROR;
    case yunlink::ErrorCode::kBindError:
        return YUNLINK_RESULT_BIND_ERROR;
    case yunlink::ErrorCode::kListenError:
        return YUNLINK_RESULT_LISTEN_ERROR;
    case yunlink::ErrorCode::kConnectError:
        return YUNLINK_RESULT_CONNECT_ERROR;
    case yunlink::ErrorCode::kTimeout:
        return YUNLINK_RESULT_TIMEOUT;
    case yunlink::ErrorCode::kEncodeError:
        return YUNLINK_RESULT_ENCODE_ERROR;
    case yunlink::ErrorCode::kDecodeError:
        return YUNLINK_RESULT_DECODE_ERROR;
    case yunlink::ErrorCode::kChecksumMismatch:
        return YUNLINK_RESULT_CHECKSUM_MISMATCH;
    case yunlink::ErrorCode::kInvalidHeader:
        return YUNLINK_RESULT_INVALID_HEADER;
    case yunlink::ErrorCode::kRuntimeStopped:
        return YUNLINK_RESULT_RUNTIME_STOPPED;
    case yunlink::ErrorCode::kProtocolMismatch:
        return YUNLINK_RESULT_PROTOCOL_MISMATCH;
    case yunlink::ErrorCode::kUnauthorized:
        return YUNLINK_RESULT_UNAUTHORIZED;
    case yunlink::ErrorCode::kRejected:
        return YUNLINK_RESULT_REJECTED;
    case yunlink::ErrorCode::kInternal:
        return YUNLINK_RESULT_INTERNAL;
    }
    return YUNLINK_RESULT_INTERNAL;
}

yunlink::TargetSelector to_target_selector(const yunlink_target_selector_t& target) {
    const auto type = static_cast<yunlink::AgentType>(target.target_type);
    switch (static_cast<yunlink_target_scope_t>(target.scope)) {
    case YUNLINK_TARGET_SCOPE_ENTITY:
        return yunlink::TargetSelector::for_entity(type, target.entity_id);
    case YUNLINK_TARGET_SCOPE_GROUP:
        return yunlink::TargetSelector::for_group(type, target.group_id);
    case YUNLINK_TARGET_SCOPE_BROADCAST:
        return yunlink::TargetSelector::broadcast(type);
    }
    return yunlink::TargetSelector::broadcast(yunlink::AgentType::kUnknown);
}

yunlink_target_selector_t to_c_target_selector(const yunlink::TargetSelector& target) {
    yunlink_target_selector_t out{};
    out.struct_size = sizeof(out);
    out.scope = static_cast<uint8_t>(target.scope);
    out.target_type = static_cast<uint8_t>(target.target_type);
    out.group_id = target.group_id;
    out.entity_id = target.target_ids.empty() ? 0 : target.target_ids.front();
    return out;
}

void to_c_peer(const std::string& peer_id, yunlink_peer_t* out_peer) {
    if (out_peer == nullptr) {
        return;
    }
    std::memset(out_peer, 0, sizeof(*out_peer));
    safe_copy(out_peer->id, sizeof(out_peer->id), peer_id);
}

yunlink::RuntimeConfig to_runtime_config(const yunlink_runtime_config_t& cfg) {
    yunlink::RuntimeConfig out;
    out.udp_bind_port = cfg.udp_bind_port;
    out.udp_target_port = cfg.udp_target_port;
    out.tcp_listen_port = cfg.tcp_listen_port;
    out.connect_timeout_ms = cfg.connect_timeout_ms;
    out.io_poll_interval_ms = cfg.io_poll_interval_ms;
    out.max_buffer_bytes_per_peer = cfg.max_buffer_bytes_per_peer;
    out.multicast_group = cfg.multicast_group[0] != '\0' ? cfg.multicast_group : "224.1.1.1";
    out.self_identity.agent_type = static_cast<yunlink::AgentType>(cfg.self_identity.agent_type);
    out.self_identity.agent_id = cfg.self_identity.agent_id;
    out.self_identity.role = static_cast<yunlink::EndpointRole>(cfg.self_identity.role);
    out.capability_flags = cfg.capability_flags;
    if (cfg.shared_secret[0] != '\0') {
        out.shared_secret = cfg.shared_secret;
    }
    return out;
}

yunlink_identity_t to_c_identity(const yunlink::EndpointIdentity& identity) {
    yunlink_identity_t out{};
    out.agent_type = static_cast<uint8_t>(identity.agent_type);
    out.agent_id = identity.agent_id;
    out.role = static_cast<uint8_t>(identity.role);
    return out;
}

void push_event(yunlink_runtime_t* runtime, const yunlink_runtime_event_t& event) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->queue.push_back(event);
}

void clear_queue(yunlink_runtime_t* runtime) {
    std::lock_guard<std::mutex> lock(runtime->mu);
    runtime->queue.clear();
}

void subscribe_runtime_events(yunlink_runtime_t* runtime) {
    auto& bus = runtime->runtime.event_bus();
    runtime->tok_error = bus.subscribe_error([runtime](const yunlink::ErrorEvent& ev) {
        yunlink_runtime_event_t out{};
        out.type = YUNLINK_RUNTIME_EVENT_ERROR;
        out.data.error.code = static_cast<uint16_t>(to_result(ev.code));
        out.data.error.transport = static_cast<uint8_t>(ev.transport);
        out.data.error.peer_port = ev.peer.port;
        safe_copy(out.data.error.peer_id, sizeof(out.data.error.peer_id), ev.peer.id);
        safe_copy(out.data.error.peer_ip, sizeof(out.data.error.peer_ip), ev.peer.ip);
        safe_copy(out.data.error.message, sizeof(out.data.error.message), ev.message);
        push_event(runtime, out);
    });

    runtime->tok_link = bus.subscribe_link([runtime](const yunlink::LinkEvent& ev) {
        yunlink_runtime_event_t out{};
        out.type = YUNLINK_RUNTIME_EVENT_LINK;
        out.data.link.transport = static_cast<uint8_t>(ev.transport);
        out.data.link.is_up = ev.is_up ? 1 : 0;
        out.data.link.peer_port = ev.peer.port;
        safe_copy(out.data.link.peer_id, sizeof(out.data.link.peer_id), ev.peer.id);
        safe_copy(out.data.link.peer_ip, sizeof(out.data.link.peer_ip), ev.peer.ip);
        push_event(runtime, out);
    });

    runtime->tok_vehicle_core =
        runtime->runtime.state_subscriber().subscribe_vehicle_core([runtime](
                                                                       const yunlink::
                                                                           TypedMessage<
                                                                               yunlink::
                                                                                   VehicleCoreState>&
                                                                               msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE;
            out.data.vehicle_core_state.session_id = msg.envelope.session_id;
            out.data.vehicle_core_state.message_id = msg.envelope.message_id;
            out.data.vehicle_core_state.correlation_id = msg.envelope.correlation_id;
            out.data.vehicle_core_state.source_type =
                static_cast<uint8_t>(msg.envelope.source.agent_type);
            out.data.vehicle_core_state.source_id = msg.envelope.source.agent_id;
            out.data.vehicle_core_state.source_role =
                static_cast<uint8_t>(msg.envelope.source.role);
            out.data.vehicle_core_state.armed = msg.payload.armed ? 1 : 0;
            out.data.vehicle_core_state.nav_mode = msg.payload.nav_mode;
            out.data.vehicle_core_state.x_m = msg.payload.x_m;
            out.data.vehicle_core_state.y_m = msg.payload.y_m;
            out.data.vehicle_core_state.z_m = msg.payload.z_m;
            out.data.vehicle_core_state.vx_mps = msg.payload.vx_mps;
            out.data.vehicle_core_state.vy_mps = msg.payload.vy_mps;
            out.data.vehicle_core_state.vz_mps = msg.payload.vz_mps;
            out.data.vehicle_core_state.battery_percent = msg.payload.battery_percent;
            push_event(runtime, out);
        });

    runtime->tok_vehicle_event =
        runtime->runtime.event_subscriber().subscribe_vehicle_event([runtime](
                                                                        const yunlink::
                                                                            TypedMessage<
                                                                                yunlink::
                                                                                    VehicleEvent>&
                                                                                msg) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_VEHICLE_EVENT;
            out.data.vehicle_event.session_id = msg.envelope.session_id;
            out.data.vehicle_event.message_id = msg.envelope.message_id;
            out.data.vehicle_event.correlation_id = msg.envelope.correlation_id;
            out.data.vehicle_event.kind = static_cast<uint8_t>(msg.payload.kind);
            out.data.vehicle_event.severity = msg.payload.severity;
            safe_copy(out.data.vehicle_event.detail,
                      sizeof(out.data.vehicle_event.detail),
                      msg.payload.detail);
            push_event(runtime, out);
        });

    runtime->tok_command_result = runtime->runtime.event_subscriber().subscribe_command_results(
        [runtime](const yunlink::CommandResultView& view) {
            yunlink_runtime_event_t out{};
            out.type = YUNLINK_RUNTIME_EVENT_COMMAND_RESULT;
            out.data.command_result.session_id = view.envelope.session_id;
            out.data.command_result.message_id = view.envelope.message_id;
            out.data.command_result.correlation_id = view.envelope.correlation_id;
            out.data.command_result.command_kind = static_cast<uint16_t>(view.payload.command_kind);
            out.data.command_result.phase = static_cast<uint8_t>(view.payload.phase);
            out.data.command_result.result_code = view.payload.result_code;
            out.data.command_result.progress_percent = view.payload.progress_percent;
            safe_copy(out.data.command_result.detail,
                      sizeof(out.data.command_result.detail),
                      view.payload.detail);
            push_event(runtime, out);
        });
}

void unsubscribe_runtime_events(yunlink_runtime_t* runtime) {
    if (runtime->tok_error != 0) {
        runtime->runtime.event_bus().unsubscribe(runtime->tok_error);
        runtime->tok_error = 0;
    }
    if (runtime->tok_link != 0) {
        runtime->runtime.event_bus().unsubscribe(runtime->tok_link);
        runtime->tok_link = 0;
    }
    if (runtime->tok_vehicle_core != 0) {
        runtime->runtime.state_subscriber().unsubscribe(runtime->tok_vehicle_core);
        runtime->tok_vehicle_core = 0;
    }
    if (runtime->tok_vehicle_event != 0) {
        runtime->runtime.event_subscriber().unsubscribe(runtime->tok_vehicle_event);
        runtime->tok_vehicle_event = 0;
    }
    if (runtime->tok_command_result != 0) {
        runtime->runtime.event_subscriber().unsubscribe(runtime->tok_command_result);
        runtime->tok_command_result = 0;
    }
}

bool validate_input_runtime(yunlink_runtime_t* runtime) {
    return runtime != nullptr;
}

bool validate_peer(const yunlink_peer_t* peer) {
    return peer != nullptr && peer->id[0] != '\0';
}

bool validate_session(const yunlink_session_t* session) {
    return session != nullptr && session->session_id != 0;
}

bool validate_target(const yunlink_target_selector_t* target) {
    return target != nullptr && target->struct_size == sizeof(*target);
}

void fill_command_handle(const yunlink::CommandHandle& in, yunlink_command_handle_t* out) {
    if (out == nullptr) {
        return;
    }
    std::memset(out, 0, sizeof(*out));
    out->session_id = in.session_id;
    out->message_id = in.message_id;
    out->correlation_id = in.correlation_id;
    out->target = to_c_target_selector(in.target);
}

}  // namespace

extern "C" {

uint32_t yunlink_ffi_abi_version(void) {
    return YUNLINK_FFI_ABI_VERSION;
}

const char* yunlink_result_name(yunlink_result_t result) {
    switch (result) {
    case YUNLINK_RESULT_OK:
        return "YUNLINK_RESULT_OK";
    case YUNLINK_RESULT_INVALID_ARGUMENT:
        return "YUNLINK_RESULT_INVALID_ARGUMENT";
    case YUNLINK_RESULT_SOCKET_ERROR:
        return "YUNLINK_RESULT_SOCKET_ERROR";
    case YUNLINK_RESULT_BIND_ERROR:
        return "YUNLINK_RESULT_BIND_ERROR";
    case YUNLINK_RESULT_LISTEN_ERROR:
        return "YUNLINK_RESULT_LISTEN_ERROR";
    case YUNLINK_RESULT_CONNECT_ERROR:
        return "YUNLINK_RESULT_CONNECT_ERROR";
    case YUNLINK_RESULT_TIMEOUT:
        return "YUNLINK_RESULT_TIMEOUT";
    case YUNLINK_RESULT_ENCODE_ERROR:
        return "YUNLINK_RESULT_ENCODE_ERROR";
    case YUNLINK_RESULT_DECODE_ERROR:
        return "YUNLINK_RESULT_DECODE_ERROR";
    case YUNLINK_RESULT_CHECKSUM_MISMATCH:
        return "YUNLINK_RESULT_CHECKSUM_MISMATCH";
    case YUNLINK_RESULT_INVALID_HEADER:
        return "YUNLINK_RESULT_INVALID_HEADER";
    case YUNLINK_RESULT_RUNTIME_STOPPED:
        return "YUNLINK_RESULT_RUNTIME_STOPPED";
    case YUNLINK_RESULT_PROTOCOL_MISMATCH:
        return "YUNLINK_RESULT_PROTOCOL_MISMATCH";
    case YUNLINK_RESULT_UNAUTHORIZED:
        return "YUNLINK_RESULT_UNAUTHORIZED";
    case YUNLINK_RESULT_REJECTED:
        return "YUNLINK_RESULT_REJECTED";
    case YUNLINK_RESULT_INTERNAL:
        return "YUNLINK_RESULT_INTERNAL";
    case YUNLINK_RESULT_NOT_FOUND:
        return "YUNLINK_RESULT_NOT_FOUND";
    }
    return "YUNLINK_RESULT_UNKNOWN";
}

yunlink_result_t yunlink_runtime_create(yunlink_runtime_t** out_runtime) {
    if (out_runtime == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    *out_runtime = new yunlink_runtime();
    return YUNLINK_RESULT_OK;
}

void yunlink_runtime_destroy(yunlink_runtime_t* runtime) {
    if (!runtime) {
        return;
    }
    yunlink_runtime_stop(runtime);
    delete runtime;
}

yunlink_result_t yunlink_runtime_start(yunlink_runtime_t* runtime,
                                       const yunlink_runtime_config_t* cfg) {
    if (!validate_input_runtime(runtime) || cfg == nullptr || cfg->struct_size != sizeof(*cfg)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (runtime->started) {
        return YUNLINK_RESULT_OK;
    }
    clear_queue(runtime);
    const auto result = to_result(runtime->runtime.start(to_runtime_config(*cfg)));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    subscribe_runtime_events(runtime);
    runtime->started = true;
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_runtime_stop(yunlink_runtime_t* runtime) {
    if (!validate_input_runtime(runtime)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (!runtime->started) {
        clear_queue(runtime);
        return YUNLINK_RESULT_OK;
    }
    unsubscribe_runtime_events(runtime);
    runtime->runtime.stop();
    runtime->started = false;
    clear_queue(runtime);
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_peer_connect(yunlink_runtime_t* runtime,
                                      const char* ip,
                                      uint16_t port,
                                      yunlink_peer_t* out_peer) {
    if (!validate_input_runtime(runtime) || ip == nullptr || out_peer == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::string peer_id;
    const auto result = to_result(runtime->runtime.tcp_clients().connect_peer(ip, port, &peer_id));
    if (result != YUNLINK_RESULT_OK) {
        return result;
    }
    to_c_peer(peer_id, out_peer);
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_session_open(yunlink_runtime_t* runtime,
                                      const yunlink_peer_t* peer,
                                      const char* node_name,
                                      yunlink_session_t* out_session) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || node_name == nullptr ||
        out_session == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (!runtime->started) {
        return YUNLINK_RESULT_RUNTIME_STOPPED;
    }
    out_session->session_id = runtime->runtime.session_client().open_active_session(peer->id, node_name);
    return out_session->session_id == 0 ? YUNLINK_RESULT_INTERNAL : YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_session_describe(yunlink_runtime_t* runtime,
                                          const yunlink_session_t* session,
                                          yunlink_session_info_t* out_info) {
    if (!validate_input_runtime(runtime) || !validate_session(session) || out_info == nullptr ||
        out_info->struct_size != sizeof(*out_info)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::SessionDescriptor desc{};
    if (!runtime->runtime.session_server().describe_session(session->session_id, &desc)) {
        return YUNLINK_RESULT_NOT_FOUND;
    }

    const size_t struct_size = out_info->struct_size;
    std::memset(out_info, 0, sizeof(*out_info));
    out_info->struct_size = struct_size;
    out_info->session_id = desc.session_id;
    out_info->state = static_cast<uint8_t>(desc.state);
    out_info->remote_identity = to_c_identity(desc.remote_identity);
    to_c_peer(desc.peer.id, &out_info->peer);
    out_info->capability_flags = desc.capability_flags;
    safe_copy(out_info->node_name, sizeof(out_info->node_name), desc.node_name);
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_authority_request(yunlink_runtime_t* runtime,
                                           const yunlink_peer_t* peer,
                                           const yunlink_session_t* session,
                                           const yunlink_target_selector_t* target,
                                           uint8_t source,
                                           uint32_t lease_ttl_ms,
                                           uint8_t allow_preempt) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    return to_result(runtime->runtime.request_authority(peer->id,
                                                        session->session_id,
                                                        to_target_selector(*target),
                                                        static_cast<yunlink::ControlSource>(source),
                                                        lease_ttl_ms,
                                                        allow_preempt != 0));
}

yunlink_result_t yunlink_authority_renew(yunlink_runtime_t* runtime,
                                         const yunlink_peer_t* peer,
                                         const yunlink_session_t* session,
                                         const yunlink_target_selector_t* target,
                                         uint8_t source,
                                         uint32_t lease_ttl_ms) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    return to_result(runtime->runtime.renew_authority(peer->id,
                                                      session->session_id,
                                                      to_target_selector(*target),
                                                      static_cast<yunlink::ControlSource>(source),
                                                      lease_ttl_ms));
}

yunlink_result_t yunlink_authority_release(yunlink_runtime_t* runtime,
                                           const yunlink_peer_t* peer,
                                           const yunlink_session_t* session,
                                           const yunlink_target_selector_t* target) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    return to_result(runtime->runtime.release_authority(
        peer->id, session->session_id, to_target_selector(*target)));
}

yunlink_result_t yunlink_authority_current(yunlink_runtime_t* runtime,
                                           yunlink_authority_lease_t* out_lease) {
    if (!validate_input_runtime(runtime) || out_lease == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::AuthorityLease lease{};
    if (!runtime->runtime.current_authority(&lease)) {
        return YUNLINK_RESULT_NOT_FOUND;
    }
    std::memset(out_lease, 0, sizeof(*out_lease));
    out_lease->state = static_cast<uint8_t>(lease.state);
    out_lease->session_id = lease.session_id;
    out_lease->target = to_c_target_selector(lease.target);
    out_lease->source = static_cast<uint8_t>(lease.source);
    out_lease->lease_ttl_ms = lease.lease_ttl_ms;
    out_lease->expires_at_ms = lease.expires_at_ms;
    to_c_peer(lease.peer.id, &out_lease->peer);
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_command_publish_takeoff(yunlink_runtime_t* runtime,
                                                 const yunlink_peer_t* peer,
                                                 const yunlink_session_t* session,
                                                 const yunlink_target_selector_t* target,
                                                 const yunlink_takeoff_command_t* payload,
                                                 yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::TakeoffCommand native{};
    native.relative_height_m = payload->relative_height_m;
    native.max_velocity_mps = payload->max_velocity_mps;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_takeoff(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_land(yunlink_runtime_t* runtime,
                                              const yunlink_peer_t* peer,
                                              const yunlink_session_t* session,
                                              const yunlink_target_selector_t* target,
                                              const yunlink_land_command_t* payload,
                                              yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::LandCommand native{};
    native.max_velocity_mps = payload->max_velocity_mps;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_land(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_return(yunlink_runtime_t* runtime,
                                                const yunlink_peer_t* peer,
                                                const yunlink_session_t* session,
                                                const yunlink_target_selector_t* target,
                                                const yunlink_return_command_t* payload,
                                                yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::ReturnCommand native{};
    native.loiter_before_return_s = payload->loiter_before_return_s;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_return(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_goto(yunlink_runtime_t* runtime,
                                              const yunlink_peer_t* peer,
                                              const yunlink_session_t* session,
                                              const yunlink_target_selector_t* target,
                                              const yunlink_goto_command_t* payload,
                                              yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::GotoCommand native{};
    native.x_m = payload->x_m;
    native.y_m = payload->y_m;
    native.z_m = payload->z_m;
    native.yaw_rad = payload->yaw_rad;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_goto(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_command_publish_velocity_setpoint(
    yunlink_runtime_t* runtime,
    const yunlink_peer_t* peer,
    const yunlink_session_t* session,
    const yunlink_target_selector_t* target,
    const yunlink_velocity_setpoint_command_t* payload,
    yunlink_command_handle_t* out_handle) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_session(session) ||
        !validate_target(target) || payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::VelocitySetpointCommand native{};
    native.vx_mps = payload->vx_mps;
    native.vy_mps = payload->vy_mps;
    native.vz_mps = payload->vz_mps;
    native.yaw_rate_radps = payload->yaw_rate_radps;
    native.body_frame = payload->body_frame != 0;
    yunlink::CommandHandle handle{};
    const auto result = to_result(runtime->runtime.command_publisher().publish_velocity_setpoint(
        peer->id, session->session_id, to_target_selector(*target), native, &handle));
    if (result == YUNLINK_RESULT_OK) {
        fill_command_handle(handle, out_handle);
    }
    return result;
}

yunlink_result_t yunlink_publish_vehicle_core_state(yunlink_runtime_t* runtime,
                                                    const yunlink_peer_t* peer,
                                                    const yunlink_target_selector_t* target,
                                                    const yunlink_vehicle_core_state_t* payload,
                                                    uint64_t session_id) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer) || !validate_target(target) ||
        payload == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    yunlink::VehicleCoreState native{};
    native.armed = payload->armed != 0;
    native.nav_mode = payload->nav_mode;
    native.x_m = payload->x_m;
    native.y_m = payload->y_m;
    native.z_m = payload->z_m;
    native.vx_mps = payload->vx_mps;
    native.vy_mps = payload->vy_mps;
    native.vz_mps = payload->vz_mps;
    native.battery_percent = payload->battery_percent;
    return to_result(runtime->runtime.publish_vehicle_core_state(
        peer->id, to_target_selector(*target), native, session_id));
}

yunlink_result_t yunlink_runtime_poll_event(yunlink_runtime_t* runtime,
                                            yunlink_runtime_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    if (runtime->queue.empty()) {
        std::memset(out_event, 0, sizeof(*out_event));
        out_event->type = YUNLINK_RUNTIME_EVENT_NONE;
        return YUNLINK_RESULT_OK;
    }
    *out_event = runtime->queue.front();
    runtime->queue.pop_front();
    return YUNLINK_RESULT_OK;
}

yunlink_result_t yunlink_runtime_poll_command_result(yunlink_runtime_t* runtime,
                                                     yunlink_command_result_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    for (auto it = runtime->queue.begin(); it != runtime->queue.end(); ++it) {
        if (it->type != YUNLINK_RUNTIME_EVENT_COMMAND_RESULT) {
            continue;
        }
        *out_event = it->data.command_result;
        runtime->queue.erase(it);
        return YUNLINK_RESULT_OK;
    }
    std::memset(out_event, 0, sizeof(*out_event));
    return YUNLINK_RESULT_NOT_FOUND;
}

yunlink_result_t
yunlink_runtime_poll_vehicle_core_state(yunlink_runtime_t* runtime,
                                        yunlink_vehicle_core_state_event_t* out_event) {
    if (!validate_input_runtime(runtime) || out_event == nullptr) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    std::lock_guard<std::mutex> lock(runtime->mu);
    for (auto it = runtime->queue.begin(); it != runtime->queue.end(); ++it) {
        if (it->type != YUNLINK_RUNTIME_EVENT_VEHICLE_CORE_STATE) {
            continue;
        }
        *out_event = it->data.vehicle_core_state;
        runtime->queue.erase(it);
        return YUNLINK_RESULT_OK;
    }
    std::memset(out_event, 0, sizeof(*out_event));
    return YUNLINK_RESULT_NOT_FOUND;
}

}  // extern "C"
