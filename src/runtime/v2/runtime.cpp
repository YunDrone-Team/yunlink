#include "runtime_internal.hpp"

#include <chrono>

namespace yunlink::v2 {
namespace {

std::error_code connect_with_timeout(asio::ip::tcp::socket& socket,
                                     const asio::ip::tcp::endpoint& endpoint,
                                     int timeout_ms) {
    auto& io = static_cast<asio::io_context&>(socket.get_executor().context());
    std::error_code result = asio::error::would_block;
    bool timed_out = false;
    asio::steady_timer timer(io);
    socket.async_connect(endpoint, [&](const std::error_code& error) {
        result = error;
        std::error_code ignored;
        timer.cancel(ignored);
    });
    timer.expires_after(std::chrono::milliseconds(timeout_ms > 0 ? timeout_ms : 1));
    timer.async_wait([&](const std::error_code& error) {
        if (!error) {
            timed_out = true;
            std::error_code ignored;
            socket.cancel(ignored);
            socket.close(ignored);
        }
    });
    io.run();
    io.restart();
    return timed_out ? asio::error::timed_out : result;
}

void close_connection(const std::shared_ptr<RuntimeConnection>& connection) {
    connection->running.store(false);
    if (connection->socket) {
        std::error_code ignored;
        connection->socket->cancel(ignored);
        connection->socket->close(ignored);
    }
    if (connection->receive_thread.joinable() &&
        connection->receive_thread.get_id() != std::this_thread::get_id()) {
        connection->receive_thread.join();
    }
}

}  // namespace

Runtime::Runtime() : impl_(std::make_unique<Impl>()) {}

Runtime::~Runtime() {
    stop();
}

ErrorCode Runtime::start(const RuntimeConfig& config) {
    if (impl_->running.load()) {
        return ErrorCode::kOk;
    }
    if (!valid_uid(config.endpoint_uid)) {
        return ErrorCode::kInvalidArgument;
    }
    for (const auto& entity : config.entities) {
        if (!valid_uid(entity.entity_uid)) {
            return ErrorCode::kInvalidArgument;
        }
    }
    for (const auto& profile : config.profiles) {
        if (!valid_profile_id(profile.profile_id) || profile.major == 0) {
            return ErrorCode::kInvalidArgument;
        }
    }
    impl_->config = config;
    impl_->acceptor = std::make_unique<asio::ip::tcp::acceptor>(impl_->accept_io);
    std::error_code error;
    impl_->acceptor->open(asio::ip::tcp::v4(), error);
    if (error) {
        return ErrorCode::kInternal;
    }
    impl_->acceptor->set_option(asio::socket_base::reuse_address(true), error);
    impl_->acceptor->bind(asio::ip::tcp::endpoint(asio::ip::tcp::v4(), config.tcp_listen_port),
                          error);
    if (error) {
        impl_->acceptor.reset();
        return ErrorCode::kInternal;
    }
    impl_->acceptor->listen(asio::socket_base::max_listen_connections, error);
    impl_->acceptor->non_blocking(true, error);
    if (error) {
        impl_->acceptor.reset();
        return ErrorCode::kInternal;
    }
    impl_->running.store(true);
    impl_->accept_thread = std::thread([this]() {
        while (impl_->running.load()) {
            auto socket = std::make_shared<asio::ip::tcp::socket>(impl_->accept_io);
            std::error_code error;
            impl_->acceptor->accept(*socket, error);
            if (error) {
                if (error == asio::error::would_block || error == asio::error::try_again) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(
                        impl_->config.io_poll_interval_ms > 0 ? impl_->config.io_poll_interval_ms
                                                              : 1));
                    continue;
                }
                if (!impl_->running.load()) {
                    break;
                }
                continue;
            }
            const auto endpoint = socket->remote_endpoint(error);
            if (error) {
                continue;
            }
            auto connection = std::make_shared<RuntimeConnection>();
            connection->peer.ip = endpoint.address().to_string();
            connection->peer.port = endpoint.port();
            connection->peer.id = runtime_peer_id(connection->peer.ip, connection->peer.port);
            connection->socket = socket;
            connection->running.store(true);
            socket->non_blocking(true, error);
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                impl_->connections[connection->peer.id] = connection;
            }
            runtime_emit(
                impl_.get(),
                {RuntimeEventKind::kLink, connection->peer, {}, {}, ErrorCode::kOk, {}, true});
            connection->receive_thread = std::thread(runtime_receive_loop, impl_.get(), connection);
        }
    });
    return ErrorCode::kOk;
}

void Runtime::stop() {
    if (!impl_->running.exchange(false)) {
        return;
    }
    if (impl_->acceptor) {
        std::error_code ignored;
        impl_->acceptor->cancel(ignored);
        impl_->acceptor->close(ignored);
    }
    if (impl_->accept_thread.joinable()) {
        impl_->accept_thread.join();
    }
    std::vector<std::shared_ptr<RuntimeConnection>> connections;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        for (const auto& entry : impl_->connections) {
            connections.push_back(entry.second);
        }
        impl_->connections.clear();
        impl_->sessions.clear();
        impl_->authority_leases.clear();
    }
    for (const auto& connection : connections) {
        close_connection(connection);
    }
    impl_->acceptor.reset();
}

bool Runtime::running() const {
    return impl_->running.load();
}

ErrorCode Runtime::connect_peer(const std::string& ip, uint16_t port, Peer* out) {
    if (!running()) {
        return ErrorCode::kRejected;
    }
    std::error_code error;
    const auto address = asio::ip::make_address(ip, error);
    if (error || port == 0) {
        return ErrorCode::kInvalidArgument;
    }
    const std::string peer_id = runtime_peer_id(ip, port);
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto existing = impl_->connections.find(peer_id);
        if (existing != impl_->connections.end() && existing->second->running.load()) {
            if (out != nullptr) {
                *out = existing->second->peer;
            }
            return ErrorCode::kOk;
        }
    }
    auto connection = std::make_shared<RuntimeConnection>();
    connection->socket = std::make_shared<asio::ip::tcp::socket>(connection->io);
    connection->socket->open(asio::ip::tcp::v4(), error);
    if (error) {
        return ErrorCode::kInternal;
    }
    error = connect_with_timeout(*connection->socket,
                                 asio::ip::tcp::endpoint(address, port),
                                 impl_->config.connect_timeout_ms);
    if (error) {
        return error == asio::error::timed_out ? ErrorCode::kTimeout : ErrorCode::kNotFound;
    }
    connection->peer = {peer_id, ip, port};
    connection->running.store(true);
    connection->socket->non_blocking(true, error);
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        impl_->connections[peer_id] = connection;
    }
    runtime_emit(impl_.get(),
                 {RuntimeEventKind::kLink, connection->peer, {}, {}, ErrorCode::kOk, {}, true});
    connection->receive_thread = std::thread(runtime_receive_loop, impl_.get(), connection);
    if (out != nullptr) {
        *out = connection->peer;
    }
    return ErrorCode::kOk;
}

void Runtime::close_peer(const std::string& peer_id) {
    std::shared_ptr<RuntimeConnection> connection;
    {
        std::lock_guard<std::mutex> lock(impl_->mutex);
        const auto it = impl_->connections.find(peer_id);
        if (it == impl_->connections.end()) {
            return;
        }
        connection = it->second;
        impl_->connections.erase(it);
    }
    close_connection(connection);
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

}  // namespace yunlink::v2
