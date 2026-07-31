#include "runtime_internal.hpp"

#include <set>

namespace yunlink::v2 {

ErrorCode Runtime::set_entities(std::vector<EntityDescriptor> entities) {
    for (const auto& entity : entities) {
        if (!valid_uid(entity.entity_uid)) {
            return ErrorCode::kInvalidArgument;
        }
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    std::set<std::string> entity_uids;
    for (const auto& entity : entities) {
        entity_uids.insert(entity.entity_uid);
    }
    for (auto lease = impl_->authority_leases.begin();
         lease != impl_->authority_leases.end();) {
        if (entity_uids.count(lease->first.first) == 0U) {
            lease = impl_->authority_leases.erase(lease);
        } else {
            ++lease;
        }
    }
    impl_->config.entities = std::move(entities);
    return ErrorCode::kOk;
}

ErrorCode Runtime::send(const std::string& peer_id, const Envelope& envelope) {
    std::shared_ptr<RuntimeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it = impl_->connections.find(peer_id);
        if (it == impl_->connections.end()) {
            return ErrorCode::kNotFound;
        }
        connection = it->second;
    }
    const Bytes bytes = impl_->codec.encode(envelope);
    if (bytes.empty()) {
        return ErrorCode::kEncodeError;
    }
    return runtime_write(connection, bytes) ? ErrorCode::kOk : ErrorCode::kInternal;
}

ErrorCode Runtime::publish(const std::string& peer_id,
                           uint64_t session_id,
                           MessageFamily family,
                           uint8_t operation,
                           const TargetSelector& target,
                           const TypeRef& type,
                           const Bytes& payload,
                           MessageHandle* out,
                           uint64_t correlation_id,
                           uint32_t ttl_ms,
                           QosClass qos,
                           const std::string& source_entity_uid) {
    Envelope envelope;
    envelope.family = family;
    envelope.operation = operation;
    envelope.qos_class = qos;
    envelope.session_id = session_id;
    envelope.message_id = impl_->next_message_id.fetch_add(1);
    envelope.correlation_id = correlation_id;
    envelope.source = {impl_->config.endpoint_uid, source_entity_uid};
    envelope.target = target;
    envelope.type = type;
    envelope.created_at_ms = runtime_now_ms();
    envelope.ttl_ms = ttl_ms;
    envelope.payload = payload;
    const ErrorCode result = send(peer_id, envelope);
    if (result == ErrorCode::kOk && out != nullptr) {
        *out = {session_id, envelope.message_id, correlation_id};
    }
    return result;
}

size_t Runtime::subscribe(EventHandler handler) {
    if (!handler) {
        return 0;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    const size_t token = impl_->next_subscription++;
    impl_->handlers[token] = std::move(handler);
    return token;
}

void Runtime::unsubscribe(size_t token) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->handlers.erase(token);
}

std::vector<SessionInfo> Runtime::sessions() const {
    std::vector<SessionInfo> result;
    std::lock_guard<std::mutex> lock(impl_->mutex);
    for (const auto& entry : impl_->sessions) {
        result.push_back(entry.second);
    }
    return result;
}

bool Runtime::session(const std::string& peer_id, uint64_t session_id, SessionInfo* out) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    const auto it = impl_->sessions.find({peer_id, session_id});
    if (it == impl_->sessions.end()) {
        return false;
    }
    if (out != nullptr) {
        *out = it->second;
    }
    return true;
}

uint16_t Runtime::listening_port() const {
    return impl_->listening_port.load();
}

}  // namespace yunlink::v2
