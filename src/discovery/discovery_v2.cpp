#include "yunlink/discovery/discovery_v2.hpp"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>

#include <asio.hpp>

namespace yunlink::v2 {
namespace {

constexpr std::array<uint8_t, 4> kQueryMagic{{'Y', 'L', 'Q', '2'}};
constexpr std::array<uint8_t, 4> kReplyMagic{{'Y', 'L', 'R', '2'}};
constexpr size_t kMaxDiscoveryBytes = 8192;
constexpr uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr uint64_t kFnvPrime = 1099511628211ULL;

uint64_t now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                     std::chrono::system_clock::now().time_since_epoch())
                                     .count());
}

uint64_t auth_tag(const std::string& secret, const uint8_t* data, size_t size) {
    uint64_t value = kFnvOffset;
    for (const unsigned char byte : secret) {
        value = (value ^ byte) * kFnvPrime;
    }
    for (size_t index = 0; index < size; ++index) {
        value = (value ^ data[index]) * kFnvPrime;
    }
    return value;
}

class Writer {
  public:
    void bytes(const uint8_t* data, size_t size) {
        value_.insert(value_.end(), data, data + size);
    }
    void u8(uint8_t value) {
        value_.push_back(value);
    }
    void u16(uint16_t value) {
        value_.push_back(static_cast<uint8_t>(value >> 8U));
        value_.push_back(static_cast<uint8_t>(value));
    }
    void u64(uint64_t value) {
        for (int shift = 56; shift >= 0; shift -= 8) {
            value_.push_back(static_cast<uint8_t>(value >> shift));
        }
    }
    bool text(const std::string& value) {
        if (value.size() > UINT16_MAX) {
            valid_ = false;
            return false;
        }
        u16(static_cast<uint16_t>(value.size()));
        value_.insert(value_.end(), value.begin(), value.end());
        return true;
    }
    Bytes finish(const std::string& secret) {
        if (!valid_ || value_.size() + sizeof(uint64_t) > kMaxDiscoveryBytes) {
            return {};
        }
        u64(auth_tag(secret, value_.data(), value_.size()));
        return std::move(value_);
    }

  private:
    Bytes value_;
    bool valid_ = true;
};

class Reader {
  public:
    Reader(const Bytes& value, size_t payload_size) : value_(value), payload_size_(payload_size) {}
    bool magic(const std::array<uint8_t, 4>& expected) {
        if (cursor_ + expected.size() > payload_size_ ||
            !std::equal(expected.begin(), expected.end(), value_.begin() + cursor_)) {
            return false;
        }
        cursor_ += expected.size();
        return true;
    }
    bool u8(uint8_t* value) {
        if (value == nullptr || cursor_ + 1 > payload_size_) {
            return false;
        }
        *value = value_[cursor_++];
        return true;
    }
    bool u16(uint16_t* value) {
        if (value == nullptr || cursor_ + 2 > payload_size_) {
            return false;
        }
        *value = static_cast<uint16_t>(value_[cursor_] << 8U) |
                 static_cast<uint16_t>(value_[cursor_ + 1]);
        cursor_ += 2;
        return true;
    }
    bool u64(uint64_t* value) {
        if (value == nullptr || cursor_ + 8 > payload_size_) {
            return false;
        }
        *value = 0;
        for (int shift = 56; shift >= 0; shift -= 8) {
            *value |= static_cast<uint64_t>(value_[cursor_++]) << shift;
        }
        return true;
    }
    bool text(std::string* value) {
        uint16_t size = 0;
        if (value == nullptr || !u16(&size) || cursor_ + size > payload_size_) {
            return false;
        }
        value->assign(reinterpret_cast<const char*>(value_.data() + cursor_), size);
        cursor_ += size;
        return true;
    }
    bool done() const {
        return cursor_ == payload_size_;
    }

  private:
    const Bytes& value_;
    size_t payload_size_ = 0;
    size_t cursor_ = 0;
};

bool authenticated(const Bytes& bytes, const std::string& secret) {
    if (bytes.size() < sizeof(uint64_t)) {
        return false;
    }
    const size_t payload_size = bytes.size() - sizeof(uint64_t);
    uint64_t supplied = 0;
    for (size_t index = payload_size; index < bytes.size(); ++index) {
        supplied = (supplied << 8U) | bytes[index];
    }
    return supplied == auth_tag(secret, bytes.data(), payload_size);
}

bool valid_advertisement(const DiscoveryAdvertisement& value) {
    if (!valid_uid(value.endpoint_uid) || value.tcp_listen_port == 0 ||
        value.capabilities.size() > UINT8_MAX || value.profiles.size() > UINT8_MAX ||
        value.entities.size() > UINT8_MAX) {
        return false;
    }
    return std::all_of(value.profiles.begin(),
                       value.profiles.end(),
                       [](const auto& profile) {
                           return valid_profile_id(profile.profile_id) && profile.major > 0;
                       }) &&
           std::all_of(value.entities.begin(), value.entities.end(), [](const auto& entity) {
               return valid_uid(entity.entity_uid);
           });
}

}  // namespace

Bytes encode_discovery_query(const DiscoveryQuery& query, const std::string& shared_secret) {
    if (query.nonce == 0 || query.response_window_ms == 0) {
        return {};
    }
    Writer writer;
    writer.bytes(kQueryMagic.data(), kQueryMagic.size());
    writer.u64(query.nonce);
    writer.u16(query.response_window_ms);
    return writer.finish(shared_secret);
}

bool decode_discovery_query(const Bytes& bytes,
                            const std::string& shared_secret,
                            DiscoveryQuery* query) {
    if (query == nullptr || bytes.size() != 22 || !authenticated(bytes, shared_secret)) {
        return false;
    }
    Reader reader(bytes, bytes.size() - sizeof(uint64_t));
    DiscoveryQuery parsed;
    if (!reader.magic(kQueryMagic) || !reader.u64(&parsed.nonce) ||
        !reader.u16(&parsed.response_window_ms) || !reader.done() || parsed.nonce == 0 ||
        parsed.response_window_ms == 0) {
        return false;
    }
    *query = parsed;
    return true;
}

Bytes encode_discovery_reply(const DiscoveryQuery& query,
                             const DiscoveryAdvertisement& advertisement,
                             const std::string& shared_secret) {
    if (query.nonce == 0 || !valid_advertisement(advertisement)) {
        return {};
    }
    Writer writer;
    writer.bytes(kReplyMagic.data(), kReplyMagic.size());
    writer.u64(query.nonce);
    writer.u8(kProtocolMajor);
    writer.u8(kHeaderVersion);
    writer.u16(kSchemaVersion);
    writer.text(advertisement.endpoint_uid);
    writer.text(advertisement.display_name);
    writer.u16(advertisement.tcp_listen_port);
    writer.u8(static_cast<uint8_t>(advertisement.capabilities.size()));
    for (const auto& capability : advertisement.capabilities) {
        writer.text(capability);
    }
    writer.u8(static_cast<uint8_t>(advertisement.profiles.size()));
    for (const auto& profile : advertisement.profiles) {
        writer.text(profile.profile_id);
        writer.u16(profile.major);
        writer.u16(profile.minor);
        writer.text(profile.schema_digest);
    }
    writer.u8(static_cast<uint8_t>(advertisement.entities.size()));
    for (const auto& entity : advertisement.entities) {
        writer.text(entity.entity_uid);
        writer.text(entity.kind);
        writer.text(entity.display_name);
        writer.u8(static_cast<uint8_t>(entity.availability));
    }
    writer.u64(advertisement.started_at_ms);
    writer.u64(advertisement.sequence);
    return writer.finish(shared_secret);
}

bool decode_discovery_reply(const Bytes& bytes,
                            const std::string& shared_secret,
                            uint64_t expected_nonce,
                            DiscoveryAdvertisement* advertisement) {
    if (advertisement == nullptr || bytes.size() > kMaxDiscoveryBytes ||
        !authenticated(bytes, shared_secret)) {
        return false;
    }
    Reader reader(bytes, bytes.size() - sizeof(uint64_t));
    DiscoveryAdvertisement parsed;
    uint64_t nonce = 0;
    uint8_t protocol = 0;
    uint8_t header = 0;
    uint16_t schema = 0;
    uint8_t count = 0;
    if (!reader.magic(kReplyMagic) || !reader.u64(&nonce) || nonce != expected_nonce ||
        !reader.u8(&protocol) || protocol != kProtocolMajor || !reader.u8(&header) ||
        header != kHeaderVersion || !reader.u16(&schema) || schema != kSchemaVersion ||
        !reader.text(&parsed.endpoint_uid) || !reader.text(&parsed.display_name) ||
        !reader.u16(&parsed.tcp_listen_port) || !reader.u8(&count)) {
        return false;
    }
    parsed.capabilities.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        std::string capability;
        if (!reader.text(&capability)) {
            return false;
        }
        parsed.capabilities.push_back(std::move(capability));
    }
    if (!reader.u8(&count)) {
        return false;
    }
    parsed.profiles.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        ProfileDescriptor profile;
        if (!reader.text(&profile.profile_id) || !reader.u16(&profile.major) ||
            !reader.u16(&profile.minor) || !reader.text(&profile.schema_digest)) {
            return false;
        }
        parsed.profiles.push_back(std::move(profile));
    }
    if (!reader.u8(&count)) {
        return false;
    }
    parsed.entities.reserve(count);
    for (uint8_t index = 0; index < count; ++index) {
        DiscoveryEntitySummary entity;
        uint8_t availability = 0;
        if (!reader.text(&entity.entity_uid) || !reader.text(&entity.kind) ||
            !reader.text(&entity.display_name) || !reader.u8(&availability) ||
            availability > static_cast<uint8_t>(Availability::kOffline)) {
            return false;
        }
        entity.availability = static_cast<Availability>(availability);
        parsed.entities.push_back(std::move(entity));
    }
    if (!reader.u64(&parsed.started_at_ms) || !reader.u64(&parsed.sequence) || !reader.done() ||
        !valid_advertisement(parsed)) {
        return false;
    }
    *advertisement = std::move(parsed);
    return true;
}

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
    if (impl_->running.load() || bind_port == 0 || !valid_advertisement(advertisement)) {
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
    if (!valid_advertisement(advertisement)) {
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
