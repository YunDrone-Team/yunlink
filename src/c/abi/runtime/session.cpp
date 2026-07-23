/**
 * @file src/c/abi/session.cpp
 * @brief C ABI peer and session functions.
 */

#include "../internal.hpp"

#include <cstring>
#include <string>

using namespace yunlink_c_abi;

extern "C" {

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

yunlink_result_t yunlink_peer_disconnect(yunlink_runtime_t* runtime, const yunlink_peer_t* peer) {
    if (!validate_input_runtime(runtime) || !validate_peer(peer)) {
        return YUNLINK_RESULT_INVALID_ARGUMENT;
    }
    if (!runtime->started) {
        return YUNLINK_RESULT_RUNTIME_STOPPED;
    }
    runtime->runtime.tcp_clients().close_peer(peer->id);
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
    out_session->session_id =
        runtime->runtime.session_client().open_active_session(peer->id, node_name);
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

}  // extern "C"
