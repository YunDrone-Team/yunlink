/**
 * @file include/yunlink/core/semantic/session_authority_types.hpp
 * @brief Semantic session and authority payload models.
 */

#ifndef YUNLINK_CORE_SEMANTIC_SESSION_AUTHORITY_TYPES_HPP
#define YUNLINK_CORE_SEMANTIC_SESSION_AUTHORITY_TYPES_HPP

#include <cstdint>
#include <string>

#include "yunlink/core/semantic/message_ids.hpp"
#include "yunlink/core/types.hpp"

namespace yunlink {

struct SessionHello {
    std::string node_name;
    uint32_t capability_flags = 0;
    uint16_t udp_bind_port = 0;
};

struct SessionAuthenticate {
    std::string shared_secret;
};

struct SessionCapabilities {
    uint32_t capability_flags = 0;
    uint16_t udp_bind_port = 0;
};

struct SessionReady {
    uint8_t accepted_protocol_major = 1;
};

struct AuthorityRequest {
    AuthorityAction action = AuthorityAction::kClaim;
    ControlSource source = ControlSource::kUnknown;
    uint32_t lease_ttl_ms = 0;
    bool allow_preempt = false;
};

struct AuthorityStatus {
    AuthorityState state = AuthorityState::kObserver;
    uint64_t session_id = 0;
    uint32_t lease_ttl_ms = 0;
    uint16_t reason_code = 0;
    std::string detail;
};

struct SessionDescriptor {
    uint64_t session_id = 0;
    SessionState state = SessionState::kDiscovered;
    // Active is a transport lifecycle state. Consumers that authorize privileged
    // operations must also require a completed shared-secret authentication.
    bool authenticated = false;
    // Distinguish locally initiated handshakes from inbound legacy handshakes
    // when Ready arrives before the capabilities acknowledgement.
    bool initiated_locally = false;
    EndpointIdentity remote_identity;
    PeerInfo peer;
    PeerInfo udp_peer;
    uint32_t capability_flags = 0;
    std::string node_name;
};

struct AuthorityLease {
    AuthorityState state = AuthorityState::kObserver;
    uint64_t session_id = 0;
    TargetSelector target;
    ControlSource source = ControlSource::kUnknown;
    uint32_t lease_ttl_ms = 0;
    uint64_t expires_at_ms = 0;
    PeerInfo peer;
};

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_SESSION_AUTHORITY_TYPES_HPP
