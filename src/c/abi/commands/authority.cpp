/**
 * @file src/c/abi/authority.cpp
 * @brief C ABI authority functions.
 */

#include "../internal.hpp"

#include <cstring>

using namespace yunlink_c_abi;

extern "C" {

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

}  // extern "C"
