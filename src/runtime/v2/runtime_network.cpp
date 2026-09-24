#include "runtime_internal.hpp"

#include <algorithm>
#include <chrono>

namespace yunlink::v2 {
namespace {

constexpr size_t kReliableOrderedReserveFraction = 4U;
constexpr size_t kWriteQuantumBytes = static_cast<size_t>(16U) * 1024U;
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

uint64_t read_u64(const uint8_t* data) {
    uint64_t value = 0;
    for (unsigned shift = 0; shift < 64; shift += 8) {
        value |= static_cast<uint64_t>(data[shift / 8]) << shift;
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

bool qos_may_drop(QosClass qos) {
    return qos == QosClass::kBestEffort || qos == QosClass::kBulk;
}

bool may_drop_congested_frame(QosClass qos, size_t bytes_written) {
    return bytes_written == 0 && qos_may_drop(qos);
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

SocketWriteResult write_frame_chunks(
    const Bytes& bytes,
    const std::function<size_t(const uint8_t*, size_t, std::error_code&)>& send_chunk,
    const std::function<bool()>& running,
    size_t retry_limit) {
    if (bytes.empty() || !running()) {
        return {SocketWriteStatus::kDisconnected, 0, "socket unavailable"};
    }
    std::error_code error;
    size_t offset = 0;
    size_t retry_count = 0;
    while (offset < bytes.size() && running()) {
        const size_t remaining = bytes.size() - offset;
        const size_t chunk = remaining > kWriteQuantumBytes ? kWriteQuantumBytes : remaining;
        const size_t sent = send_chunk(bytes.data() + offset, chunk, error);
        offset += sent;
        if (!error) {
            if (sent == 0U) {
                return {SocketWriteStatus::kDisconnected, offset, "zero-byte write"};
            }
            retry_count = 0;
            continue;
        }
        if (error != asio::error::would_block && error != asio::error::try_again) {
            return {SocketWriteStatus::kDisconnected, offset, error.message()};
        }
        if (sent != 0U) {
            retry_count = 0;
        }
        if (++retry_count > retry_limit) {
            return {SocketWriteStatus::kCongested, offset, error.message()};
        }
        error.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    if (offset == bytes.size()) {
        return {SocketWriteStatus::kComplete, offset, {}};
    }
    return {running() ? SocketWriteStatus::kCongested
                                      : SocketWriteStatus::kDisconnected,
            offset, "socket stopped during write"};
}

SocketWriteResult write_socket(const std::shared_ptr<RuntimeConnection>& connection,
                               const Bytes& bytes) {
    if (!connection || !connection->running.load() || !connection->socket) {
        return {SocketWriteStatus::kDisconnected, 0, "socket unavailable"};
    }
    return write_frame_chunks(
        bytes,
        [&connection](const uint8_t* data, size_t size, std::error_code& error) {
            std::lock_guard<std::mutex> lock(connection->socket_mutex);
            if (!connection->socket || !connection->running.load()) {
                error = asio::error::operation_aborted;
                return size_t{0};
            }
            return connection->socket->write_some(asio::buffer(data, size), error);
        },
        [&connection]() { return connection->running.load(); },
        kCongestionRetryLimit);
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

void runtime_send_loop(Runtime::Impl* impl,
                       const std::shared_ptr<RuntimeConnection>& connection) {
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
        const SocketWriteResult result = write_socket(connection, frame.bytes);
        if (result.status == SocketWriteStatus::kComplete) {
            continue;
        }
        if (result.status == SocketWriteStatus::kCongested &&
            may_drop_congested_frame(frame.qos, result.bytes_written)) {
            continue;
        }
        size_t queued_bytes = 0;
        {
            std::lock_guard<std::mutex> lock(connection->send_mutex);
            queued_bytes = connection->queued_bytes;
        }
        bool lossy_channel = false;
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            lossy_channel = impl->lane_owner.find(connection->peer.id) != impl->lane_owner.end();
        }
        runtime_emit(impl,
                     {RuntimeEventKind::kError, connection->peer, {}, {},
                      ErrorCode::kInternal,
                      "socket write failed: channel=" +
                          std::string(lossy_channel ? "lossy" : "control") +
                          " session_id=" +
                          std::to_string(frame.bytes.size() >= 28
                                             ? read_u64(frame.bytes.data() + 20)
                                             : 0) +
                          " bytes_written=" +
                          std::to_string(result.bytes_written) +
                          " frame_bytes=" + std::to_string(frame.bytes.size()) +
                          " queued_bytes=" + std::to_string(queued_bytes) +
                          " qos=" + std::to_string(static_cast<unsigned>(frame.qos)) +
                          " reason=" + result.error});
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
