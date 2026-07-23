/**
 * @file src/discovery/endpoint_advertiser.cpp
 * @brief UDP endpoint discovery advertiser.
 */

#include "yunlink/discovery/endpoint_discovery.hpp"

#include <array>
#include <chrono>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

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
    std::thread recv_thread;
    std::mutex send_mu;
    std::mutex advertisement_mu;
    EndpointAdvertisement advertisement;
    bool has_advertisement{false};
    std::mutex rate_mu;
    std::unordered_map<std::string, uint64_t> handled_nonces;
    std::unordered_map<std::string, std::deque<uint64_t>> source_queries;
    std::deque<uint64_t> global_queries;
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

    impl_->socket->bind(asio::ip::udp::endpoint(asio::ip::udp::v4(), config.discovery_port), ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("advertiser bind failed: " + ec.message());
        return ErrorCode::kBindError;
    }
    impl_->socket->non_blocking(true, ec);
    if (ec) {
        impl_->socket.reset();
        set_last_error("advertiser non-blocking failed: " + ec.message());
        return ErrorCode::kSocketError;
    }

    impl_->running.store(true);
    impl_->recv_thread = std::thread(&EndpointAdvertiser::recv_loop, this);
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
    }
    if (impl_->recv_thread.joinable()) {
        impl_->recv_thread.join();
    }
    impl_->socket.reset();
}

bool EndpointAdvertiser::is_running() const {
    return impl_->running.load();
}

void EndpointAdvertiser::set_advertisement(const EndpointAdvertisement& advertisement) {
    std::lock_guard<std::mutex> lock(impl_->advertisement_mu);
    impl_->advertisement = advertisement;
    impl_->has_advertisement = true;
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

    set_advertisement(advertisement);
    const ByteBuffer payload = encode_endpoint_advertisement(advertisement);
    const asio::ip::udp::endpoint endpoint(address, impl_->config.discovery_port);
    std::lock_guard<std::mutex> lock(impl_->send_mu);
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

void EndpointAdvertiser::recv_loop() {
    std::array<uint8_t, 256> buffer{};
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
            set_last_error("advertiser receive failed: " + ec.message());
            continue;
        }

        if (source.port() == 0 || source.address().is_multicast() ||
            source.address().is_unspecified() ||
            (source.address().is_v4() && source.address().to_v4().to_uint() == 0xffffffffU)) {
            continue;
        }
        EndpointDiscoveryQuery query{};
        std::string error;
        if (!decode_endpoint_discovery_query(ByteBuffer(buffer.begin(), buffer.begin() + received),
                                             impl_->config.shared_secret,
                                             &query,
                                             &error)) {
            continue;
        }

        EndpointAdvertisement advertisement;
        {
            std::lock_guard<std::mutex> lock(impl_->advertisement_mu);
            if (!impl_->has_advertisement) {
                continue;
            }
            advertisement = impl_->advertisement;
        }
        const std::string source_ip = source.address().to_string(ec);
        if (ec) {
            continue;
        }
        const uint64_t now =
            static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                      std::chrono::steady_clock::now().time_since_epoch())
                                      .count());
        {
            std::lock_guard<std::mutex> lock(impl_->rate_mu);
            for (auto it = impl_->handled_nonces.begin(); it != impl_->handled_nonces.end();) {
                if (now >= it->second + 10000U) {
                    it = impl_->handled_nonces.erase(it);
                } else {
                    ++it;
                }
            }
            for (auto it = impl_->source_queries.begin(); it != impl_->source_queries.end();) {
                auto& timestamps = it->second;
                while (!timestamps.empty() && now >= timestamps.front() + 1000U) {
                    timestamps.pop_front();
                }
                if (timestamps.empty()) {
                    it = impl_->source_queries.erase(it);
                } else {
                    ++it;
                }
            }
            while (!impl_->global_queries.empty() && now >= impl_->global_queries.front() + 1000U) {
                impl_->global_queries.pop_front();
            }
            if (impl_->global_queries.size() >= impl_->config.query_rate_limit_per_sec) {
                continue;
            }
            auto& seen = impl_->handled_nonces[std::to_string(query.nonce)];
            if (seen != 0 && now < seen + 10000U) {
                continue;
            }
            auto& source_queries = impl_->source_queries[source_ip];
            if (source_queries.size() >= impl_->config.query_rate_limit_per_sec) {
                continue;
            }
            impl_->global_queries.push_back(now);
            source_queries.push_back(now);
            seen = now;
        }

        const uint64_t mix =
            query.nonce ^ (std::hash<std::string>{}(advertisement.endpoint_id) << 1U);
        const uint16_t window =
            std::min<uint16_t>(query.response_window_ms, impl_->config.query_response_window_ms);
        const uint16_t delay_ms = static_cast<uint16_t>(mix % std::max<uint16_t>(window, 50U));
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        advertisement.sequence += 1U;
        // Preserve schema-1 active discovery for older listeners, then send a
        // second, optional reply carrying the bounded entity directory. Legacy
        // listeners accept the first packet; newer listeners replace it with
        // the same-endpoint summary packet when it arrives.
        auto legacy_advertisement = advertisement;
        legacy_advertisement.managed_entity_count_known = false;
        legacy_advertisement.managed_entity_count = 0U;
        legacy_advertisement.managed_entities.clear();
        const ByteBuffer legacy_reply = encode_endpoint_discovery_reply(
            query.nonce, legacy_advertisement, impl_->config.shared_secret);
        if (legacy_reply.empty()) {
            set_last_error("discovery reply encoding failed");
            continue;
        }
        const ByteBuffer summary_reply =
            advertisement.managed_entity_count_known
                ? encode_endpoint_discovery_reply(
                      query.nonce, advertisement, impl_->config.shared_secret)
                : ByteBuffer{};
        if (advertisement.managed_entity_count_known && summary_reply.empty()) {
            set_last_error("discovery managed-entity reply encoding failed");
        }
        std::lock_guard<std::mutex> lock(impl_->send_mu);
        impl_->socket->send_to(asio::buffer(legacy_reply), source, 0, ec);
        if (ec && !socket_closed(ec)) {
            set_last_error("discovery reply failed: " + ec.message());
            continue;
        }
        if (!summary_reply.empty()) {
            impl_->socket->send_to(asio::buffer(summary_reply), source, 0, ec);
            if (ec && !socket_closed(ec)) {
                set_last_error("discovery managed-entity reply failed: " + ec.message());
            }
        }
    }
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
