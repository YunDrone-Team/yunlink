#include "runtime_internal.hpp"

#include <algorithm>

#include "yunlink/core/core_messages_v2.hpp"

namespace yunlink::v2 {
namespace {

TypeRef session_type(const char* type_name) {
    return {"yunlink.core", 2, 0, type_name};
}

ProfileDescriptor negotiate_profile(const ProfileDescriptor& local,
                                    const ProfileDescriptor& remote,
                                    bool* digest_conflict) {
    ProfileDescriptor result;
    if (local.profile_id != remote.profile_id || local.major != remote.major) {
        return result;
    }
    const uint16_t negotiated_minor = std::min(local.minor, remote.minor);
    if (local.minor == remote.minor && local.schema_digest != remote.schema_digest) {
        *digest_conflict = true;
        return result;
    }
    result = local.minor <= remote.minor ? local : remote;
    result.minor = negotiated_minor;
    return result;
}

void emit_session(Runtime::Impl* impl, const Peer& peer, const SessionInfo& session) {
    runtime_emit(impl, {RuntimeEventKind::kSession, peer, {}, session, ErrorCode::kOk, {}, false});
}

void send_session(Runtime::Impl* impl,
                  const std::string& peer_id,
                  uint64_t session_id,
                  SessionOperation operation,
                  const char* type_name,
                  const Bytes& payload,
                  uint64_t correlation_id = 0) {
    Envelope envelope;
    envelope.family = MessageFamily::kSession;
    envelope.operation = static_cast<uint8_t>(operation);
    envelope.session_id = session_id;
    envelope.message_id = impl->next_message_id.fetch_add(1);
    envelope.correlation_id = correlation_id;
    envelope.source.endpoint_uid = impl->config.endpoint_uid;
    envelope.target = TargetSelector::broadcast();
    envelope.type = session_type(type_name);
    envelope.created_at_ms = runtime_now_ms();
    envelope.ttl_ms = 5000;
    envelope.payload = payload;
    std::shared_ptr<RuntimeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        const auto it = impl->connections.find(peer_id);
        if (it != impl->connections.end()) {
            connection = it->second;
        }
    }
    if (connection) {
        runtime_write(connection, impl->codec.encode(envelope));
    }
}

}  // namespace

uint64_t Runtime::open_session(const std::string& peer_id) {
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        if (impl_->connections.find(peer_id) == impl_->connections.end()) {
            return 0;
        }
    }
    const uint64_t session_id = impl_->next_session_id.fetch_add(1);
    SessionInfo info;
    info.session_id = session_id;
    info.peer_id = peer_id;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->sessions[{peer_id, session_id}] = info;
    }
    send_session(impl_.get(),
                 peer_id,
                 session_id,
                 SessionOperation::kHello,
                 "session.hello",
                 encode_text(impl_->config.endpoint_uid));
    send_session(impl_.get(),
                 peer_id,
                 session_id,
                 SessionOperation::kAuthenticate,
                 "session.authenticate",
                 encode_text(impl_->config.shared_secret));
    send_session(impl_.get(),
                 peer_id,
                 session_id,
                 SessionOperation::kProfiles,
                 "session.profiles",
                 encode_profile_list(impl_->config.profiles));
    send_session(impl_.get(), peer_id, session_id, SessionOperation::kReady, "session.ready", {});
    return session_id;
}

void runtime_handle_envelope(Runtime::Impl* impl, const Peer& peer, const Envelope& envelope) {
    if (envelope.family != MessageFamily::kSession) {
        bool authorized = false;
        bool profile_supported = false;
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            const auto it = impl->sessions.find({peer.id, envelope.session_id});
            if (it != impl->sessions.end() && it->second.state == SessionState::kActive &&
                it->second.remote_endpoint_uid == envelope.source.endpoint_uid) {
                std::vector<std::string> entity_uids;
                entity_uids.reserve(impl->config.entities.size());
                for (const auto& entity : impl->config.entities) {
                    entity_uids.push_back(entity.entity_uid);
                }
                authorized = envelope.target.matches(
                    impl->config.endpoint_uid, entity_uids, impl->config.group_uids);
                profile_supported =
                    envelope.type.is_core() ||
                    it->second.supports_profile(envelope.type.profile_id,
                                                envelope.type.major,
                                                envelope.type.minor);
            }
        }
        if (!authorized) {
            runtime_emit(impl,
                         {RuntimeEventKind::kError,
                          peer,
                          envelope,
                          {},
                          ErrorCode::kUnauthorized,
                          "inactive session, source mismatch, or target mismatch"});
            return;
        }
        if (!profile_supported) {
            runtime_emit(impl,
                         {RuntimeEventKind::kError,
                          peer,
                          envelope,
                          {},
                          ErrorCode::kUnsupported,
                          "message type exceeds the negotiated profile version"});
            return;
        }
        if (runtime_handle_authority(impl, peer, envelope)) {
            return;
        }
        std::string authority_error;
        if (!runtime_action_authorized(impl, peer, envelope, &authority_error)) {
            runtime_emit(impl,
                         {RuntimeEventKind::kError,
                          peer,
                          envelope,
                          {},
                          ErrorCode::kUnauthorized,
                          authority_error});
            return;
        }
        runtime_emit(impl, {RuntimeEventKind::kEnvelope, peer, envelope});
        return;
    }

    SessionInfo snapshot;
    bool reply_profiles = false;
    bool reply_ready = false;
    bool source_mismatch = false;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        SessionInfo& session = impl->sessions[{peer.id, envelope.session_id}];
        session.session_id = envelope.session_id;
        session.peer_id = peer.id;
        if (session.remote_endpoint_uid.empty()) {
            session.remote_endpoint_uid = envelope.source.endpoint_uid;
        } else if (session.remote_endpoint_uid != envelope.source.endpoint_uid) {
            session.state = SessionState::kInvalid;
            snapshot = session;
            source_mismatch = true;
        }
        const auto operation = static_cast<SessionOperation>(envelope.operation);
        if (source_mismatch) {
            // Keep the invalid state and notify after releasing the runtime mutex.
        } else if (operation == SessionOperation::kHello) {
            std::string endpoint_uid;
            if (!decode_text(envelope.payload, &endpoint_uid) ||
                endpoint_uid != envelope.source.endpoint_uid || !valid_uid(endpoint_uid)) {
                session.state = SessionState::kInvalid;
            }
        } else if (operation == SessionOperation::kAuthenticate) {
            std::string secret;
            session.authenticated =
                decode_text(envelope.payload, &secret) && secret == impl->config.shared_secret;
            session.state =
                session.authenticated ? SessionState::kAuthenticated : SessionState::kInvalid;
        } else if (operation == SessionOperation::kProfiles) {
            std::vector<ProfileDescriptor> remote_profiles;
            if (!decode_profile_list(envelope.payload, &remote_profiles)) {
                session.state = SessionState::kInvalid;
            } else {
                session.negotiated_profiles.clear();
                session.rejected_profiles.clear();
                for (const auto& remote : remote_profiles) {
                    const auto local = std::find_if(
                        impl->config.profiles.begin(),
                        impl->config.profiles.end(),
                        [&](const auto& value) { return value.profile_id == remote.profile_id; });
                    if (local == impl->config.profiles.end()) {
                        session.rejected_profiles.push_back(remote.profile_id);
                        continue;
                    }
                    bool digest_conflict = false;
                    const auto negotiated = negotiate_profile(*local, remote, &digest_conflict);
                    if (negotiated.profile_id.empty()) {
                        session.rejected_profiles.push_back(remote.profile_id);
                    } else {
                        session.negotiated_profiles[remote.profile_id] = negotiated;
                    }
                }
                if (session.state != SessionState::kInvalid) {
                    session.state = SessionState::kNegotiated;
                }
                reply_profiles = envelope.correlation_id == 0;
            }
        } else if (operation == SessionOperation::kReady) {
            if (session.state == SessionState::kNegotiated) {
                session.state = SessionState::kActive;
                // Ready follows Authenticate and Profiles on the ordered session channel.
                session.authenticated = true;
                reply_ready = envelope.correlation_id == 0;
            } else if (session.state != SessionState::kActive) {
                session.state = SessionState::kInvalid;
            }
        }
        snapshot = session;
    }
    emit_session(impl, peer, snapshot);
    if (source_mismatch) {
        return;
    }
    if (reply_profiles) {
        send_session(impl,
                     peer.id,
                     envelope.session_id,
                     SessionOperation::kProfiles,
                     "session.profiles",
                     encode_profile_list(impl->config.profiles),
                     envelope.message_id);
    }
    if (reply_ready) {
        send_session(impl,
                     peer.id,
                     envelope.session_id,
                     SessionOperation::kReady,
                     "session.ready",
                     {},
                     envelope.message_id);
    }
}

void runtime_drop_peer_state(Runtime::Impl* impl, const Peer& peer) {
    std::vector<SessionInfo> lost;
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        for (auto session = impl->sessions.begin(); session != impl->sessions.end();) {
            if (session->first.first == peer.id) {
                session->second.state = SessionState::kLost;
                lost.push_back(session->second);
                session = impl->sessions.erase(session);
            } else {
                ++session;
            }
        }
        for (auto lease = impl->authority_leases.begin(); lease != impl->authority_leases.end();) {
            if (lease->second.peer_id == peer.id) {
                lease = impl->authority_leases.erase(lease);
            } else {
                ++lease;
            }
        }
    }
    for (const auto& session : lost) {
        emit_session(impl, peer, session);
    }
}

}  // namespace yunlink::v2
