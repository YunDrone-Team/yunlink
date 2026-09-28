#include "yunlink/discovery/discovery_v2.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <map>
#include <mutex>
#include <thread>

#if !defined(_WIN32)
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

#include <asio.hpp>

namespace yunlink::v2 {

bool discovery_advertisement_is_valid(const DiscoveryAdvertisement& value);

namespace {

using Address = asio::ip::address_v4;

std::vector<Address> multicast_interfaces() {
    std::vector<Address> result;
#if !defined(_WIN32)
    ifaddrs* interfaces = nullptr;
    if (getifaddrs(&interfaces) != 0)
        return result;
    for (const ifaddrs* entry = interfaces; entry != nullptr; entry = entry->ifa_next) {
        if (!entry->ifa_addr || entry->ifa_addr->sa_family != AF_INET ||
            !(entry->ifa_flags & IFF_UP) || !(entry->ifa_flags & IFF_MULTICAST))
            continue;
        const auto* address = reinterpret_cast<const sockaddr_in*>(entry->ifa_addr);
        result.push_back(Address(ntohl(address->sin_addr.s_addr)));
    }
    freeifaddrs(interfaces);
#else
    result.push_back(Address::any());
#endif
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

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
    EventHandler event_handler;
    std::map<Address, bool> multicast_memberships;
    bool reported_missing_multicast_interface = false;
};

DiscoveryAdvertiser::DiscoveryAdvertiser() : impl_(std::make_unique<Impl>()) {}

DiscoveryAdvertiser::~DiscoveryAdvertiser() {
    stop();
}

ErrorCode DiscoveryAdvertiser::start(uint16_t bind_port,
                                     DiscoveryAdvertisement advertisement,
                                     std::string shared_secret,
                                     EventHandler event_handler) {
    if (impl_->running.load() || bind_port == 0 ||
        !discovery_advertisement_is_valid(advertisement)) {
        return ErrorCode::kInvalidArgument;
    }
    if (advertisement.started_at_ms == 0) {
        advertisement.started_at_ms = now_ms();
    }
    impl_->advertisement = std::move(advertisement);
    impl_->shared_secret = std::move(shared_secret);
    impl_->event_handler = std::move(event_handler);
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
    impl_->multicast_memberships.clear();
    impl_->running.store(true);
    impl_->thread = std::thread([this]() {
        const auto emit = [this](DiscoveryAdvertiserEventKind kind,
                                 const asio::ip::udp::endpoint& remote,
                                 ErrorCode error,
                                 const std::string& detail = std::string{}) {
            if (!impl_->event_handler)
                return;
            DiscoveryAdvertiserEvent event;
            event.kind = kind;
            event.error = error;
            std::error_code endpoint_error;
            event.remote_ip = remote.address().to_string(endpoint_error);
            if (endpoint_error)
                event.remote_ip.clear();
            event.remote_port = remote.port();
            event.detail = detail;
            try {
                impl_->event_handler(event);
            } catch (...) {
                // Diagnostics must never stop the discovery responder.
                return;
            }
        };
        const auto group = Address::from_string(kDiscoveryMulticastGroup);
        const auto refresh_multicast = [this, &emit, group]() {
            const auto interfaces = multicast_interfaces();
            if (interfaces.empty() && !impl_->reported_missing_multicast_interface) {
                emit(DiscoveryAdvertiserEventKind::kMulticastJoinFailed,
                     asio::ip::udp::endpoint(asio::ip::address_v4::any(), 0),
                     ErrorCode::kInternal,
                     "no multicast-capable IPv4 interface");
                impl_->reported_missing_multicast_interface = true;
            } else if (!interfaces.empty()) {
                impl_->reported_missing_multicast_interface = false;
            }
            for (auto it = impl_->multicast_memberships.begin();
                 it != impl_->multicast_memberships.end();) {
                if (std::find(interfaces.begin(), interfaces.end(), it->first) !=
                    interfaces.end()) {
                    ++it;
                    continue;
                }
                if (it->second) {
                    std::error_code ignored;
                    impl_->socket->set_option(asio::ip::multicast::leave_group(group, it->first),
                                              ignored);
                    emit(DiscoveryAdvertiserEventKind::kMulticastLeft,
                         asio::ip::udp::endpoint(it->first, 0),
                         ErrorCode::kOk);
                }
                it = impl_->multicast_memberships.erase(it);
            }
            for (const auto& address : interfaces) {
                const auto known = impl_->multicast_memberships.find(address);
                std::error_code join_error;
                impl_->socket->set_option(asio::ip::multicast::join_group(group, address),
                                          join_error);
                const bool already_joined = known != impl_->multicast_memberships.end() &&
                                            known->second &&
                                            join_error == asio::error::address_in_use;
                if (already_joined)
                    continue;
                const bool was_healthy =
                    known != impl_->multicast_memberships.end() && known->second;
                impl_->multicast_memberships[address] = !join_error;
                if (!join_error && !was_healthy)
                    emit(DiscoveryAdvertiserEventKind::kMulticastJoined,
                         asio::ip::udp::endpoint(address, 0),
                         ErrorCode::kOk);
                if (join_error && (known == impl_->multicast_memberships.end() || was_healthy))
                    emit(DiscoveryAdvertiserEventKind::kMulticastJoinFailed,
                         asio::ip::udp::endpoint(address, 0),
                         ErrorCode::kInternal,
                         join_error.message());
            }
        };
        refresh_multicast();
        auto next_membership_refresh = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        std::array<uint8_t, 1024> buffer{};
        while (impl_->running.load()) {
            if (std::chrono::steady_clock::now() >= next_membership_refresh) {
                refresh_multicast();
                next_membership_refresh =
                    std::chrono::steady_clock::now() + std::chrono::seconds(5);
            }
            asio::ip::udp::endpoint remote;
            std::error_code error;
            const size_t size = impl_->socket->receive_from(asio::buffer(buffer), remote, 0, error);
            if (error == asio::error::would_block || error == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
                continue;
            }
            if (error) {
                if (impl_->running.load()) {
                    emit(DiscoveryAdvertiserEventKind::kReceiveError, remote, ErrorCode::kInternal);
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                continue;
            }
            DiscoveryQuery query;
            const Bytes request(buffer.begin(), buffer.begin() + size);
            if (!decode_discovery_query(request, impl_->shared_secret, &query)) {
                emit(DiscoveryAdvertiserEventKind::kRejected, remote, ErrorCode::kUnauthorized);
                continue;
            }
            emit(DiscoveryAdvertiserEventKind::kQueryReceived, remote, ErrorCode::kOk);
            DiscoveryAdvertisement advertisement;
            {
                std::lock_guard<std::mutex> lock(impl_->mutex);
                advertisement = impl_->advertisement;
                advertisement.sequence += 1;
                impl_->advertisement.sequence = advertisement.sequence;
            }
            const Bytes reply = encode_discovery_reply(query, advertisement, impl_->shared_secret);
            if (reply.empty()) {
                emit(DiscoveryAdvertiserEventKind::kSendError, remote, ErrorCode::kEncodeError);
                continue;
            }
            impl_->socket->send_to(asio::buffer(reply), remote, 0, error);
            emit(error ? DiscoveryAdvertiserEventKind::kSendError
                       : DiscoveryAdvertiserEventKind::kReplySent,
                 remote,
                 error ? ErrorCode::kInternal : ErrorCode::kOk);
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
    impl_->multicast_memberships.clear();
    impl_->reported_missing_multicast_interface = false;
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
