/**
 * @file src/c/abi/helpers.cpp
 * @brief Shared conversion and validation helpers for the C ABI.
 */

#include "../internal.hpp"

#include <algorithm>
#include <cstring>

namespace yunlink_c_abi {

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

}  // namespace yunlink_c_abi
