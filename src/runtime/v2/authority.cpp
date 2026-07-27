#include "runtime_internal.hpp"

#include <algorithm>

#include "yunlink/core/core_messages_v2.hpp"

namespace yunlink::v2 {
namespace {

TypeRef authority_status_type() {
    return {"yunlink.core", 2, 0, "authority.status"};
}

void send_status(Runtime::Impl* impl,
                 const Peer& peer,
                 const Envelope& request,
                 const AuthorityStatus& status) {
    Envelope response;
    response.family = MessageFamily::kAuthority;
    response.operation = static_cast<uint8_t>(AuthorityOperation::kStatus);
    response.session_id = request.session_id;
    response.message_id = impl->next_message_id.fetch_add(1);
    response.correlation_id = request.message_id;
    response.source.endpoint_uid = impl->config.endpoint_uid;
    if (request.target.scope == TargetScope::kEntity && request.target.uids.size() == 1) {
        response.source.entity_uid = request.target.uids.front();
    }
    response.target = TargetSelector::endpoint(request.source.endpoint_uid);
    response.type = authority_status_type();
    response.created_at_ms = runtime_now_ms();
    response.ttl_ms = 5000;
    response.payload = encode(status);
    std::shared_ptr<RuntimeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        const auto it = impl->connections.find(peer.id);
        if (it != impl->connections.end()) {
            connection = it->second;
        }
    }
    if (connection) {
        runtime_write(connection, impl->codec.encode(response));
    }
}

}  // namespace

bool runtime_handle_authority(Runtime::Impl* impl, const Peer& peer, const Envelope& envelope) {
    if (envelope.family != MessageFamily::kAuthority ||
        envelope.operation == static_cast<uint8_t>(AuthorityOperation::kStatus)) {
        return false;
    }
    AuthorityRequest request;
    AuthorityStatus status;
    status.state = "rejected";
    status.reason_code = static_cast<uint16_t>(ErrorCode::kInvalidArgument);
    status.detail = "invalid authority request";
    if (!decode(envelope.payload, &request) || envelope.target.scope != TargetScope::kEntity ||
        envelope.target.uids.size() != 1 || request.authority_scope.empty() ||
        request.lease_ttl_ms == 0) {
        send_status(impl, peer, envelope, status);
        return true;
    }
    const auto key = std::make_pair(envelope.target.uids.front(), request.authority_scope);
    const uint64_t now = runtime_now_ms();
    status.authority_scope = request.authority_scope;
    status.lease_ttl_ms = request.lease_ttl_ms;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        auto existing = impl->authority_leases.find(key);
        if (existing != impl->authority_leases.end() && existing->second.expires_at_ms <= now) {
            existing = impl->authority_leases.erase(existing);
        }
        const auto operation = static_cast<AuthorityOperation>(envelope.operation);
        const bool owner = existing != impl->authority_leases.end() &&
                           existing->second.peer_id == peer.id &&
                           existing->second.session_id == envelope.session_id;
        if (operation == AuthorityOperation::kRelease) {
            if (owner) {
                impl->authority_leases.erase(existing);
                status.state = "released";
                status.reason_code = 0;
                status.detail = "authority released";
            } else {
                status.reason_code = static_cast<uint16_t>(ErrorCode::kUnauthorized);
                status.detail = "authority is not owned by this session";
            }
        } else if (operation == AuthorityOperation::kRenew) {
            if (owner) {
                existing->second.expires_at_ms = now + request.lease_ttl_ms;
                status.state = "controller";
                status.reason_code = 0;
                status.detail = "authority renewed";
            } else {
                status.reason_code = static_cast<uint16_t>(ErrorCode::kUnauthorized);
                status.detail = "authority cannot be renewed by this session";
            }
        } else if (operation == AuthorityOperation::kClaim) {
            if (existing == impl->authority_leases.end() || owner || request.allow_preempt) {
                impl->authority_leases[key] = {
                    peer.id, envelope.session_id, now + request.lease_ttl_ms};
                status.state = "controller";
                status.reason_code = 0;
                status.detail = owner ? "authority refreshed" : "authority granted";
            } else {
                status.reason_code = static_cast<uint16_t>(ErrorCode::kConflict);
                status.detail = "authority is held by another session";
            }
        }
    }
    send_status(impl, peer, envelope, status);
    return true;
}

bool runtime_action_authorized(Runtime::Impl* impl,
                               const Peer& peer,
                               const Envelope& envelope,
                               std::string* detail) {
    if (envelope.family != MessageFamily::kAction ||
        (envelope.operation != static_cast<uint8_t>(ActionOperation::kGoal) &&
         envelope.operation != static_cast<uint8_t>(ActionOperation::kCancel))) {
        return true;
    }
    if (envelope.target.scope != TargetScope::kEntity || envelope.target.uids.size() != 1) {
        if (detail != nullptr) {
            *detail = "actions require exactly one entity target";
        }
        return false;
    }
    const auto key = std::make_pair(envelope.target.uids.front(), envelope.type.profile_id);
    const uint64_t now = runtime_now_ms();
    std::lock_guard<std::mutex> lock(impl->mutex);
    const auto lease = impl->authority_leases.find(key);
    const bool authorized = lease != impl->authority_leases.end() &&
                            lease->second.expires_at_ms > now && lease->second.peer_id == peer.id &&
                            lease->second.session_id == envelope.session_id;
    if (!authorized && detail != nullptr) {
        *detail = "no active authority lease for entity and profile scope";
    }
    return authorized;
}

void runtime_revoke_authority(Runtime::Impl* impl,
                              const std::string& peer_id,
                              uint64_t session_id,
                              const std::vector<std::string>& entity_uids) {
    std::lock_guard<std::mutex> lock(impl->mutex);
    for (auto lease = impl->authority_leases.begin(); lease != impl->authority_leases.end();) {
        const bool requested_entity =
            entity_uids.empty() ||
            std::find(entity_uids.begin(), entity_uids.end(), lease->first.first) !=
                entity_uids.end();
        if (requested_entity && lease->second.peer_id == peer_id &&
            lease->second.session_id == session_id) {
            lease = impl->authority_leases.erase(lease);
        } else {
            ++lease;
        }
    }
}

bool Runtime::has_authority(const std::string& peer_id,
                            uint64_t session_id,
                            const std::string& entity_uid,
                            const std::string& authority_scope) const {
    const auto key = std::make_pair(entity_uid, authority_scope);
    const uint64_t now = runtime_now_ms();
    std::lock_guard<std::mutex> lock(impl_->mutex);
    const auto lease = impl_->authority_leases.find(key);
    return lease != impl_->authority_leases.end() && lease->second.peer_id == peer_id &&
           lease->second.session_id == session_id && lease->second.expires_at_ms > now;
}

void Runtime::revoke_authority(const std::string& peer_id,
                               uint64_t session_id,
                               const std::vector<std::string>& entity_uids) {
    runtime_revoke_authority(impl_.get(), peer_id, session_id, entity_uids);
}

}  // namespace yunlink::v2
