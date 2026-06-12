/**
 * @file src/discovery/endpoint_advertiser.cpp
 * @brief UDP endpoint discovery advertiser.
 */

#include "yunlink/discovery/endpoint_discovery.hpp"

#include <mutex>

#include <asio.hpp>

namespace yunlink {
namespace {

bool socket_closed(const std::error_code& ec) {
    return ec == asio::error::operation_aborted || ec == asio::error::bad_descriptor;
}

}  // namespace

struct EndpointAdvertiser::Impl {
    EndpointDiscoveryConfig config;
    std::atomic<bool> running{false};
    asio::io_context io;
    std::unique_ptr<asio::ip::udp::socket> socket;
    mutable std::mutex error_mu;
    std::string last_error;
};

EndpointAdvertiser::EndpointAdvertiser() : impl_(std::make_unique<Impl>()) {}

EndpointAdvertiser::~EndpointAdvertiser() {
    stop();
}

ErrorCode EndpointAdvertiser::start(const EndpointDiscoveryConfig& config) {
    if (impl_->running.load()) {
        return ErrorCode::kOk;
    }

    impl_->config = config;
    impl_->socket = std::make_unique<asio::ip::udp::socket>(impl_->io);

    std::error_code ec;
    impl_->socket->open(asio::ip::udp::v4(), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("advertiser open failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::broadcast(true), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("advertiser broadcast option failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->socket->set_option(asio::socket_base::reuse_address(true), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("advertiser reuse_address failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->running.store(true);
    set_last_error(std::string());
    return ErrorCode::kOk;
}

void EndpointAdvertiser::stop() {
    if (!impl_->running.exchange(false)) {
        return;
    }
    if (impl_->socket != nullptr) {
        std::error_code ec;
        impl_->socket->cancel(ec);
        impl_->socket->close(ec);
        impl_->socket.reset();
    }
}

bool EndpointAdvertiser::is_running() const {
    return impl_->running.load();
}

int EndpointAdvertiser::send(const EndpointAdvertisement& advertisement) {
    if (!impl_->running.load() || impl_->socket == nullptr) {
        set_last_error("advertiser not running");
        return -1;
    }

    std::error_code ec;
    const auto address = asio::ip::make_address(impl_->config.target_ip, ec);
    if (ec) {
        set_last_error("invalid discovery target ip: " + impl_->config.target_ip);
        return -1;
    }

    const ByteBuffer payload = encode_endpoint_advertisement(advertisement);
    const asio::ip::udp::endpoint endpoint(address, impl_->config.discovery_port);
    const std::size_t sent = impl_->socket->send_to(asio::buffer(payload), endpoint, 0, ec);
    if (ec) {
        if (!socket_closed(ec)) {
            set_last_error("advertiser send failed: " + ec.message());
        }
        return -1;
    }

    set_last_error(std::string());
    return sent > static_cast<std::size_t>(INT32_MAX) ? -1 : static_cast<int>(sent);
}

std::string EndpointAdvertiser::last_error() const {
    std::lock_guard<std::mutex> lock(impl_->error_mu);
    return impl_->last_error;
}

void EndpointAdvertiser::set_last_error(const std::string& error) {
    std::lock_guard<std::mutex> lock(impl_->error_mu);
    impl_->last_error = error;
}

}  // namespace yunlink
