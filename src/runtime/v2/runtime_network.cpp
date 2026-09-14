#include "runtime_internal.hpp"

#include <algorithm>
#include <chrono>

namespace yunlink::v2 {
namespace {

constexpr size_t kReliableOrderedReserveFraction = 4U;
constexpr size_t kWriteQuantumBytes = 16U * 1024U;
constexpr size_t kCongestionRetryLimit = 1000U;

uint16_t read_u16(const uint8_t* data) {
    return static_cast<uint16_t>(data[0]) | static_cast<uint16_t>(data[1] << 8U);
}

uint32_t read_u32(const uint8_t* data) {
    uint32_t value = 0;
    for (unsigned shift = 0; shift < 32; shift += 8) {
        value |= static_cast<uint32_t>(data[shift / 8]) << shift;
    }
    return value;
}

bool peek_frame_header(const Bytes& buffer,
                       uint16_t* header_len,
                       uint32_t* payload_len,
                       QosClass* qos) {
    if (buffer.size() < 15 || !WireCodec::has_magic(buffer.data(), buffer.size())) {
        return false;
    }
    const uint16_t parsed_header_len = read_u16(buffer.data() + 8);
    const uint32_t parsed_payload_len = read_u32(buffer.data() + 10);
    const uint8_t qos_value = buffer[14];
    if (parsed_header_len < WireCodec::kFixedHeaderSize ||
        qos_value < static_cast<uint8_t>(QosClass::kReliableOrdered) ||
        qos_value > static_cast<uint8_t>(QosClass::kBulk)) {
        return false;
    }
    *header_len = parsed_header_len;
    *payload_len = parsed_payload_len;
    *qos = static_cast<QosClass>(qos_value);
    return true;
}

bool socket_closed(const std::error_code& error) {
    return error == asio::error::operation_aborted || error == asio::error::bad_descriptor ||
           error == asio::error::eof || error == asio::error::connection_reset ||
           error == asio::error::connection_aborted;
}

bool pop_frame(Runtime::Impl* impl, Bytes* buffer, Envelope* out, ErrorCode* error) {
    while (buffer->size() >= 4) {
        size_t magic = 0;
        while (magic + 3 < buffer->size() &&
               !WireCodec::has_magic(buffer->data() + magic, buffer->size() - magic)) {
            ++magic;
        }
        if (magic + 3 >= buffer->size()) {
            buffer->clear();
            return false;
        }
        if (magic > 0) {
            buffer->erase(buffer->begin(), buffer->begin() + static_cast<long>(magic));
        }
        if (buffer->size() < 14) {
            return false;
        }
        const uint16_t header_len =
            static_cast<uint16_t>((*buffer)[8]) | static_cast<uint16_t>((*buffer)[9] << 8U);
        const uint32_t payload_len = static_cast<uint32_t>((*buffer)[10]) |
                                     (static_cast<uint32_t>((*buffer)[11]) << 8U) |
                                     (static_cast<uint32_t>((*buffer)[12]) << 16U) |
                                     (static_cast<uint32_t>((*buffer)[13]) << 24U);
        const size_t frame_len =
            static_cast<size_t>(header_len) + payload_len + WireCodec::kTrailerSize;
        if (header_len < WireCodec::kFixedHeaderSize ||
            frame_len > impl->config.max_buffer_bytes_per_peer) {
            buffer->erase(buffer->begin());
            *error = ErrorCode::kDecodeError;
            return true;
        }
        if (buffer->size() < frame_len) {
            return false;
        }
        const auto decoded = impl->codec.decode(buffer->data(), frame_len, runtime_now_ms());
        if (!decoded.ok()) {
            buffer->erase(buffer->begin(), buffer->begin() + static_cast<long>(frame_len));
            *error = decoded.code;
            return true;
        }
        *out = decoded.envelope;
        buffer->erase(buffer->begin(), buffer->begin() + static_cast<long>(decoded.consumed));
        *error = ErrorCode::kOk;
        return true;
    }
    return false;
}

}  // namespace

enum class SocketWriteStatus {
    kComplete,
    kCongested,
    kDisconnected,
};

bool qos_may_drop(QosClass qos) {
    return qos == QosClass::kBestEffort || qos == QosClass::kBulk;
}

bool recover_receive_overflow(Bytes* buffer, size_t max_bytes) {
    if (buffer == nullptr) {
        return false;
    }
    while (buffer->size() > max_bytes) {
        uint16_t header_len = 0;
        uint32_t payload_len = 0;
        QosClass qos = QosClass::kReliableOrdered;
        if (peek_frame_header(*buffer, &header_len, &payload_len, &qos)) {
            if (!qos_may_drop(qos)) {
                return false;
            }
            const size_t frame_len =
                static_cast<size_t>(header_len) + payload_len + WireCodec::kTrailerSize;
            buffer->erase(buffer->begin(),
                          buffer->begin() + static_cast<long>(std::min(buffer->size(), frame_len)));
            continue;
        }
        size_t magic = 1;
        while (magic + 3 < buffer->size() &&
               !WireCodec::has_magic(buffer->data() + magic, buffer->size() - magic)) {
            ++magic;
        }
        if (magic + 3 >= buffer->size()) {
            buffer->clear();
            return true;
        }
        buffer->erase(buffer->begin(), buffer->begin() + static_cast<long>(magic));
    }
    return true;
}

SocketWriteStatus write_socket(const std::shared_ptr<RuntimeConnection>& connection,
                               const Bytes& bytes) {
    if (!connection || !connection->running.load() || !connection->socket || bytes.empty()) {
        return SocketWriteStatus::kDisconnected;
    }
    std::error_code error;
    size_t offset = 0;
    size_t retry_count = 0;
    while (offset < bytes.size() && connection->running.load()) {
        const size_t remaining = bytes.size() - offset;
        const size_t chunk = remaining > kWriteQuantumBytes ? kWriteQuantumBytes : remaining;
        size_t sent = 0;
        {
            std::lock_guard<std::mutex> socket_lock(connection->socket_mutex);
            if (!connection->socket || !connection->running.load()) {
                return SocketWriteStatus::kDisconnected;
            }
            sent =
                connection->socket->write_some(asio::buffer(bytes.data() + offset, chunk), error);
        }
        if (!error) {
            if (sent == 0U) {
                return SocketWriteStatus::kDisconnected;
            }
            offset += sent;
            retry_count = 0;
            continue;
        }
        if (error != asio::error::would_block && error != asio::error::try_again) {
            return SocketWriteStatus::kDisconnected;
        }
        if (++retry_count > kCongestionRetryLimit) {
            return SocketWriteStatus::kCongested;
        }
        error.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (offset == bytes.size()) {
        return SocketWriteStatus::kComplete;
    }
    return connection->running.load() ? SocketWriteStatus::kCongested
                                      : SocketWriteStatus::kDisconnected;
}

size_t qos_index(QosClass qos) {
    switch (qos) {
    case QosClass::kReliableOrdered:
        return 0;
    case QosClass::kReliableLatest:
        return 1;
    case QosClass::kBestEffort:
        return 2;
    case QosClass::kBulk:
        return 3;
    }
    return 0;
}

bool drop_oldest(RuntimeConnection* connection, size_t queue_index) {
    auto& queue = connection->send_queues[queue_index];
    if (queue.empty()) {
        return false;
    }
    connection->queued_bytes -= queue.front().bytes.size();
    queue.pop_front();
    return true;
}

size_t enqueue_limit(const RuntimeConnection* connection, QosClass qos);

bool make_room(RuntimeConnection* connection, size_t bytes, QosClass qos) {
    const size_t limit = enqueue_limit(connection, qos);
    while (connection->queued_bytes + bytes > limit) {
        if (drop_oldest(connection, 3) || drop_oldest(connection, 2)) {
            continue;
        }
        return false;
    }
    return true;
}

size_t enqueue_limit(const RuntimeConnection* connection, QosClass qos) {
    if (qos == QosClass::kReliableOrdered) {
        return connection->max_queued_bytes;
    }
    // Keep a bounded slice available for control-plane frames even when a
    // telemetry/media burst fills the connection queue.
    const size_t reserve = connection->max_queued_bytes / kReliableOrderedReserveFraction;
    return connection->max_queued_bytes - reserve;
}

bool runtime_enqueue(const std::shared_ptr<RuntimeConnection>& connection,
                     Bytes bytes,
                     QosClass qos,
                     std::string latest_key) {
    if (!connection || bytes.empty() || !connection->running.load()) {
        return false;
    }
    const size_t index = qos_index(qos);
    std::lock_guard<std::mutex> lock(connection->send_mutex);
    if (!connection->running.load() || bytes.size() > enqueue_limit(connection.get(), qos)) {
        return false;
    }
    if (qos == QosClass::kReliableLatest && !latest_key.empty()) {
        auto& queue = connection->send_queues[index];
        for (auto& frame : queue) {
            if (frame.latest_key == latest_key) {
                connection->queued_bytes -= frame.bytes.size();
                if (!make_room(connection.get(), bytes.size(), qos)) {
                    connection->queued_bytes += frame.bytes.size();
                    return false;
                }
                frame.bytes = std::move(bytes);
                connection->queued_bytes += frame.bytes.size();
                connection->send_condition.notify_one();
                return true;
            }
        }
    }
    if (!make_room(connection.get(), bytes.size(), qos)) {
        return false;
    }
    connection->queued_bytes += bytes.size();
    connection->send_queues[index].push_back({std::move(bytes), qos, std::move(latest_key)});
    connection->send_condition.notify_one();
    return true;
}

bool take_next_frame(RuntimeConnection* connection, RuntimeConnection::OutboundFrame* frame) {
    std::lock_guard<std::mutex> lock(connection->send_mutex);
    size_t index = 0;
    while (index < 4U && connection->send_queues[index].empty()) {
        ++index;
    }
    if (index == 4U) {
        return false;
    }
    *frame = std::move(connection->send_queues[index].front());
    connection->send_queues[index].pop_front();
    connection->queued_bytes -= frame->bytes.size();
    return true;
}

void runtime_send_loop(const std::shared_ptr<RuntimeConnection>& connection) {
    while (connection->running.load()) {
        RuntimeConnection::OutboundFrame frame;
        {
            std::unique_lock<std::mutex> lock(connection->send_mutex);
            connection->send_condition.wait(lock, [&]() {
                return !connection->running.load() || connection->queued_bytes != 0U;
            });
        }
        if (!connection->running.load() || !take_next_frame(connection.get(), &frame)) {
            continue;
        }
        const SocketWriteStatus status = write_socket(connection, frame.bytes);
        if (status == SocketWriteStatus::kComplete) {
            continue;
        }
        if (status == SocketWriteStatus::kCongested && qos_may_drop(frame.qos)) {
            continue;
        }
        connection->running.store(false);
        connection->send_condition.notify_all();
        if (connection->socket) {
            std::lock_guard<std::mutex> socket_lock(connection->socket_mutex);
            std::error_code ignored;
            connection->socket->cancel(ignored);
            connection->socket->close(ignored);
        }
        break;
    }
}

bool runtime_write(const std::shared_ptr<RuntimeConnection>& connection, const Bytes& bytes) {
    return runtime_enqueue(connection, Bytes(bytes), QosClass::kReliableOrdered);
}

void runtime_receive_loop(Runtime::Impl* impl,
                          const std::shared_ptr<RuntimeConnection>& connection) {
    std::array<uint8_t, 8192> chunk{};
    while (impl->running.load() && connection->running.load()) {
        std::error_code error;
        size_t received = 0;
        {
            std::lock_guard<std::mutex> socket_lock(connection->socket_mutex);
            if (!connection->socket || !connection->running.load()) {
                break;
            }
            received = connection->socket->read_some(asio::buffer(chunk), error);
        }
        if (error) {
            if (error == asio::error::would_block || error == asio::error::try_again) {
                std::this_thread::sleep_for(std::chrono::milliseconds(
                    impl->config.io_poll_interval_ms > 0 ? impl->config.io_poll_interval_ms : 1));
                continue;
            }
            if (!socket_closed(error)) {
                runtime_emit(impl,
                             {RuntimeEventKind::kError,
                              connection->peer,
                              {},
                              {},
                              ErrorCode::kInternal,
                              error.message()});
            }
            break;
        }
        connection->receive_buffer.insert(
            connection->receive_buffer.end(), chunk.begin(), chunk.begin() + received);
        if (connection->receive_buffer.size() > impl->config.max_buffer_bytes_per_peer) {
            if (recover_receive_overflow(&connection->receive_buffer,
                                         impl->config.max_buffer_bytes_per_peer)) {
                runtime_emit(impl,
                             {RuntimeEventKind::kError,
                              connection->peer,
                              {},
                              {},
                              ErrorCode::kDecodeError,
                              "receive buffer dropped lossy frames"});
            } else {
                runtime_emit(impl,
                             {RuntimeEventKind::kError,
                              connection->peer,
                              {},
                              {},
                              ErrorCode::kDecodeError,
                              "receive buffer limit exceeded"});
                break;
            }
        }
        while (true) {
            Envelope envelope;
            ErrorCode decode_error = ErrorCode::kOk;
            if (!pop_frame(impl, &connection->receive_buffer, &envelope, &decode_error)) {
                break;
            }
            if (decode_error != ErrorCode::kOk) {
                runtime_emit(impl,
                             {RuntimeEventKind::kError,
                              connection->peer,
                              {},
                              {},
                              decode_error,
                              "wire frame rejected"});
                continue;
            }
            runtime_handle_envelope(impl, connection->peer, envelope);
        }
    }
    connection->running.store(false);
    if (connection->socket) {
        std::lock_guard<std::mutex> socket_lock(connection->socket_mutex);
        std::error_code ignored;
        connection->socket->shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        connection->socket->close(ignored);
    }
    runtime_drop_peer_state(impl, connection->peer);
    runtime_emit(impl,
                 {RuntimeEventKind::kLink, connection->peer, {}, {}, ErrorCode::kOk, {}, false});
}

}  // namespace yunlink::v2
