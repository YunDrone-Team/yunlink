#include "runtime_internal.hpp"

#include <chrono>

namespace yunlink::v2 {
namespace {

bool socket_closed(const std::error_code& error) {
    return error == asio::error::operation_aborted || error == asio::error::bad_descriptor;
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

bool runtime_write(const std::shared_ptr<RuntimeConnection>& connection, const Bytes& bytes) {
    if (!connection || !connection->running.load() || !connection->socket || bytes.empty()) {
        return false;
    }
    std::error_code error;
    std::lock_guard<std::mutex> lock(connection->send_mutex);
    size_t offset = 0;
    size_t retry_count = 0;
    while (offset < bytes.size() && connection->running.load()) {
        const size_t sent = connection->socket->write_some(
            asio::buffer(bytes.data() + offset, bytes.size() - offset), error);
        if (!error) {
            if (sent == 0U) {
                return false;
            }
            offset += sent;
            retry_count = 0;
            continue;
        }
        if (error != asio::error::would_block && error != asio::error::try_again) {
            return false;
        }
        if (++retry_count > 1000U) {
            return false;
        }
        error.clear();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return offset == bytes.size();
}

void runtime_receive_loop(Runtime::Impl* impl,
                          const std::shared_ptr<RuntimeConnection>& connection) {
    std::array<uint8_t, 8192> chunk{};
    while (impl->running.load() && connection->running.load()) {
        std::error_code error;
        const size_t received = connection->socket->read_some(asio::buffer(chunk), error);
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
            runtime_emit(impl,
                         {RuntimeEventKind::kError,
                          connection->peer,
                          {},
                          {},
                          ErrorCode::kDecodeError,
                          "receive buffer limit exceeded"});
            break;
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
        std::error_code ignored;
        connection->socket->shutdown(asio::ip::tcp::socket::shutdown_both, ignored);
        connection->socket->close(ignored);
    }
    runtime_drop_peer_state(impl, connection->peer);
    runtime_emit(impl,
                 {RuntimeEventKind::kLink, connection->peer, {}, {}, ErrorCode::kOk, {}, false});
}

}  // namespace yunlink::v2
