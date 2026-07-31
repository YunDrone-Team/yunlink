#include "yunlink/discovery/discovery_v2.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include <asio.hpp>

namespace yunlink::v2 {

bool discovery_advertisement_is_valid(const DiscoveryAdvertisement& value);

namespace {

uint64_t now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count());
}

}  // namespace

struct DiscoveryAdvertiser::Impl {
    std::atomic<bool> running{false};
    asio::io_context io;
    std::unique_ptr<asio::ip::udp::socket> socket;
    std::thread thread;
    mutable std::mutex mutex;
    DiscoveryAdvertisement advertisement;
    std::string shared_secret;
};

DiscoveryAdvertiser::DiscoveryAdvertiser() : impl_(std::make_unique<Impl>()) {}

DiscoveryAdvertiser::~DiscoveryAdvertiser() {
    stop();
}

ErrorCode DiscoveryAdvertiser::start(uint16_t bind_port,
                                     DiscoveryAdvertisement advertisement,
                                     std::string shared_secret) {
    if (impl_->running.load() || bind_port == 0 ||
        !discovery_advertisement_is_valid(advertisement)) {
        return ErrorCode::kInvalidArgument;
    }
    if (advertisement.started_at_ms == 0) {
        advertisement.started_at_ms = now_ms();
    }
    impl_->advertisement = std::move(advertisement);
    impl_->shared_secret = std::move(shared_secret);
    impl_->socket = std::make_unique<asio::ip::udp::socket>(impl_->io);
    std::error_code error;
    impl_->socket->open(asio::ip::udp::v4(), error);
    if (!error) {
        impl_->socket->set_option(asio::socket_base::reuse_address(true), error);
    }
    if (!error) {
        impl_->socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), bind_port), error);
    }
    if (error) {
        impl_->socket.reset();
        return ErrorCode::kInternal;
    }
    impl_->socket->non_blocking(true, error);
    if (error) {
        impl_->socket.reset();
        return ErrorCode::kInternal;
    }
    impl_->running.store(true);
    impl_->thread = std::thread([this]() {
        std::array<uint8_t, 1024> buffer{};
        while (impl_->running.load()) {
            asio::ip::udp::endpoint remote;
            std::error_code error;
            const size_t size = impl_->socket->receive_from(asio::buffer(buffer), remote, 0, error);
            if (error == asio::error::would_block || error == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (error) {
                if (impl_->running.load()) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                continue;
            }
            DiscoveryQuery query;
            const Bytes request(buffer.begin(), buffer.begin() + size);
            if (!decode_discovery_query(request, impl_->shared_secret, &query)) {
                continue;
            }
            DiscoveryAdvertisement advertisement;
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                advertisement = impl_->advertisement;
                advertisement.sequence += 1;
                impl_->advertisement.sequence = advertisement.sequence;
            }
            const Bytes reply = encode_discovery_reply(query, advertisement, impl_->shared_secret);
            if (!reply.empty()) {
                impl_->socket->send_to(asio::buffer(reply), remote, 0, error);
            }
        }
    });
    return ErrorCode::kOk;
}

void DiscoveryAdvertiser::stop() {
    impl_->running.store(false);
    if (impl_->socket) {
        std::error_code ignored;
        impl_->socket->cancel(ignored);
        impl_->socket->close(ignored);
    }
    if (impl_->thread.joinable()) {
        impl_->thread.join();
    }
    impl_->socket.reset();
}

bool DiscoveryAdvertiser::running() const {
    return impl_->running.load();
}

void DiscoveryAdvertiser::set_advertisement(DiscoveryAdvertisement advertisement) {
    if (!discovery_advertisement_is_valid(advertisement)) {
        return;
    }
    std::lock_guard<std::mutex> lock(impl_->mutex);
    if (advertisement.started_at_ms == 0) {
        advertisement.started_at_ms = impl_->advertisement.started_at_ms;
    }
    advertisement.sequence = impl_->advertisement.sequence;
    impl_->advertisement = std::move(advertisement);
}

}  // namespace yunlink::v2
