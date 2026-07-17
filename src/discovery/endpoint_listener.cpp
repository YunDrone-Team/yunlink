/**
 * @file src/discovery/endpoint_listener.cpp
 * @brief UDP endpoint discovery listener.
 */

#include "yunlink/discovery/endpoint_discovery.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>

#include <asio.hpp>

namespace yunlink {
namespace {

uint64_t wall_time_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count());
}

bool socket_closed(const std::error_code& ec) {
    return ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor;
}

}  // namespace

struct EndpointListener::Impl {
    EndpointDiscoveryConfig config;
    std::atomic<bool> running{false};
    asio::io_context io;
    std::unique_ptr<asio::ip::udp::socket> socket;
    std::thread recv_thread;
    mutable std::mutex queue_mu;
    std::deque<EndpointAdvertisementPacket> queue;
    mutable std::mutex error_mu;
    std::string last_error;
};

EndpointListener::EndpointListener() : impl_(std::make_unique<Impl>()) {}

EndpointListener::~EndpointListener() {
    stop();
}

ErrorCode EndpointListener::start(const EndpointDiscoveryConfig& config) {
    if (impl_->running.load()) {
        return ErrorCode::kOk;
    }

    impl_->config = config;
    impl_->socket = std::make_unique<asio::ip::udp::socket>(impl_->io);

    std::error_code ec;
    impl_->socket->open(asio::ip::udp::v4(), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("listener open failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("listener reuse_address failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::broadcast(true), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("listener broadcast option failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), config.discovery_port), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("listener bind failed: " + ec.message());
        return ErrorCode::kBindError;
    }

    impl_->socket->non_blocking(true, ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("listener non_blocking failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    {
        std::lock_guard<std::mutex> lock(impl_->queue_mu);
        impl_->queue.clear();
    }
    set_last_error(std::string());
    impl_->running.store(true);
    impl_->recv_thread = std::thread(&EndpointListener::recv_loop, this);
    return ErrorCode::kOk;
}

void EndpointListener::stop() {
    if (!impl_->running.exchange(false)) {
        return;
    }

    if (impl_->socket != nullptr) {
        std::error_code ec;
        impl_->socket->cancel(ec);
        impl_->socket->close(ec);
    }
    if (impl_->recv_thread.joinable()) {
        impl_->recv_thread.join();
    }
    {
        std::lock_guard<std::mutex> lock(impl_->queue_mu);
        impl_->queue.clear();
    }
    impl_->socket.reset();
}

bool EndpointListener::is_running() const {
    return impl_->running.load();
}

ErrorCode EndpointListener::send_query(uint64_t nonce, uint16_t response_window_ms) {
    if (!impl_->running.load() || impl_->socket == nullptr || nonce == 0) {
        return ErrorCode::kInvalidArgument;
    }
    std::error_code ec;
    const auto address = asio::ip::make_address(impl_->config.target_ip, ec);
    if (ec) {
        set_last_error("invalid discovery target ip: " + impl_->config.target_ip);
        return ErrorCode::kInvalidArgument;
    }
    const ByteBuffer query = encode_endpoint_discovery_query(
        EndpointDiscoveryQuery{nonce, response_window_ms}, impl_->config.shared_secret);
    impl_->socket->send_to(
        asio::buffer(query), asio::ip::udp::endpoint(address, impl_->config.discovery_port), 0, ec);
    if (ec) {
        set_last_error("discovery query send failed: " + ec.message());
        return ErrorCode::kSocketError;
    }
    return ErrorCode::kOk;
}

size_t EndpointListener::drain(std::vector<EndpointAdvertisementPacket>* out) {
    if (out == nullptr) {
        return 0;
    }

    std::lock_guard<std::mutex> lock(impl_->queue_mu);
    out->clear();
    out->reserve(impl_->queue.size());
    while (!impl_->queue.empty()) {
        out->push_back(std::move(impl_->queue.front()));
        impl_->queue.pop_front();
    }
    return out->size();
}

std::string EndpointListener::last_error() const {
    std::lock_guard<std::mutex> lock(impl_->error_mu);
    return impl_->last_error;
}

void EndpointListener::recv_loop() {
    std::array<uint8_t, 2048> buffer{};

    while (impl_->running.load()) {
        std::error_code ec;
        asio::ip::udp::endpoint source;
        const std::size_t received =
            impl_->socket->receive_from(asio::buffer(buffer), source, 0, ec);

        if (ec) {
            if (ec == asio::error::would_block || ec == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    impl_->config.io_poll_interval_ms > 0 ? impl_->config.io_poll_interval_ms : 1));
                continue;
            }
            if (!impl_->running.load() || socket_closed(ec)) {
                break;
            }
            set_last_error("listener receive failed: " + ec.message());
            continue;
        }

        const ByteBuffer bytes(buffer.begin(), buffer.begin() + received);
        EndpointAdvertisement advertisement{};
        std::string error;
        uint64_t nonce = 0;
        const bool query_reply = decode_endpoint_discovery_reply(
            bytes, impl_->config.shared_secret, &nonce, &advertisement, &error);
        if (!query_reply && !decode_endpoint_advertisement(bytes, &advertisement, &error)) {
            // Queries on the shared discovery port are expected and are not listener errors.
            continue;
        }

        EndpointAdvertisementPacket packet{};
        packet.advertisement = std::move(advertisement);
        packet.source_ip = source.address().to_string(ec);
        if (ec) {
            continue;
        }
        packet.source_port = source.port();
        packet.received_at_ms = wall_time_ms();
        packet.is_query_reply = query_reply;
        packet.reply_nonce = query_reply ? nonce : 0;

        std::lock_guard<std::mutex> lock(impl_->queue_mu);
        impl_->queue.push_back(std::move(packet));
        set_last_error(std::string());
    }
}

void EndpointListener::set_last_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->error_mu);
    impl_->last_error = error;
}

}  // namespace yunlink
