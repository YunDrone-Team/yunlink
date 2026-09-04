#ifndef YUNLINK_RUNTIME_V2_RUNTIME_INTERNAL_HPP
#define YUNLINK_RUNTIME_V2_RUNTIME_INTERNAL_HPP

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

#include <asio.hpp>

#include "yunlink/core/wire_v2_codec.hpp"
#include "yunlink/runtime/runtime_v2.hpp"

namespace yunlink::v2 {

struct RuntimeConnection {
    struct OutboundFrame {
        Bytes bytes;
        QosClass qos = QosClass::kReliableOrdered;
        std::string latest_key;
    };

    Peer peer;
    std::atomic<bool> running{false};
    std::thread receive_thread;
    std::thread send_thread;
    asio::io_context io;
    std::shared_ptr<asio::ip::tcp::socket> socket;
    std::mutex send_mutex;
    std::condition_variable send_condition;
    std::deque<OutboundFrame> send_queues[4];
    size_t queued_bytes{0};
    size_t max_queued_bytes{static_cast<size_t>(16U) * 1024U * 1024U};
    Bytes receive_buffer;
};

struct Runtime::Impl {
    RuntimeConfig config;
    std::atomic<bool> running{false};
    std::atomic<uint16_t> listening_port{0};
    std::atomic<uint64_t> next_message_id{1};
    std::atomic<uint64_t> next_session_id{1};
    asio::io_context accept_io;
    std::unique_ptr<asio::ip::tcp::acceptor> acceptor;
    std::thread accept_thread;
    mutable std::mutex mutex;
    std::unordered_map<std::string, std::shared_ptr<RuntimeConnection>> connections;
    std::map<std::pair<std::string, uint64_t>, SessionInfo> sessions;
    struct AuthorityLease {
        std::string peer_id;
        uint64_t session_id = 0;
        std::chrono::steady_clock::time_point expires_at;
    };
    std::map<std::pair<std::string, std::string>, AuthorityLease> authority_leases;
    size_t next_subscription = 1;
    std::unordered_map<size_t, EventHandler> handlers;
    WireCodec codec;
};

uint64_t runtime_now_ms();
std::chrono::steady_clock::time_point runtime_steady_now();
std::string runtime_peer_id(const std::string& ip, uint16_t port);
void runtime_emit(Runtime::Impl* impl, const RuntimeEvent& event);
bool runtime_write(const std::shared_ptr<RuntimeConnection>& connection, const Bytes& bytes);
bool runtime_enqueue(const std::shared_ptr<RuntimeConnection>& connection,
                     Bytes bytes,
                     QosClass qos,
                     std::string latest_key = {});
void runtime_send_loop(const std::shared_ptr<RuntimeConnection>& connection);
void runtime_receive_loop(Runtime::Impl* impl,
                          const std::shared_ptr<RuntimeConnection>& connection);
void runtime_handle_envelope(Runtime::Impl* impl, const Peer& peer, const Envelope& envelope);
bool runtime_handle_authority(Runtime::Impl* impl, const Peer& peer, const Envelope& envelope);
bool runtime_action_authorized(Runtime::Impl* impl,
                               const Peer& peer,
                               const Envelope& envelope,
                               std::string* detail);
void runtime_revoke_authority(Runtime::Impl* impl,
                              const std::string& peer_id,
                              uint64_t session_id,
                              const std::vector<std::string>& entity_uids);
void runtime_drop_peer_state(Runtime::Impl* impl, const Peer& peer);
Bytes encode_profile_list(const std::vector<ProfileDescriptor>& profiles);
bool decode_profile_list(const Bytes& payload, std::vector<ProfileDescriptor>* profiles);
Bytes encode_text(const std::string& value);
bool decode_text(const Bytes& payload, std::string* value);

}  // namespace yunlink::v2

#endif
