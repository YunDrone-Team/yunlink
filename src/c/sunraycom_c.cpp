/**
 * @file src/c/sunraycom_c.cpp
 * @brief sunray_communication_lib source file.
 */

#include "sunraycom/c/sunraycom_c.h"

#include <algorithm>
#include <cstring>
#include <deque>
#include <mutex>

#include "sunraycom/runtime/runtime.hpp"

using sunraycom::ByteBuffer;

struct sunraycom_handle {
    sunraycom::Runtime runtime;
    std::mutex mu;
    std::deque<sunraycom_event_t> queue;
    size_t tok_envelope = 0;
    size_t tok_error = 0;
    size_t tok_link = 0;
};

namespace {

void safe_copy(char* dst, size_t cap, const std::string& src) {
    if (!dst || cap == 0) {
        return;
    }
    const size_t n = std::min(cap - 1, src.size());
    std::memcpy(dst, src.data(), n);
    dst[n] = '\0';
}

ByteBuffer to_bytes(const uint8_t* data, size_t len) {
    if (!data || len == 0) {
        return {};
    }
    return ByteBuffer(data, data + len);
}

}  // namespace

extern "C" {

sunraycom_handle_t* sunraycom_create(void) {
    return new sunraycom_handle();
}

void sunraycom_destroy(sunraycom_handle_t* handle) {
    if (!handle) {
        return;
    }
    handle->runtime.stop();
    delete handle;
}

sunraycom_status_t sunraycom_start(sunraycom_handle_t* handle,
                                   const sunraycom_runtime_config_t* cfg) {
    if (!handle) {
        return SUNRAYCOM_STATUS_ERR;
    }

    sunraycom::RuntimeConfig rc;
    if (cfg) {
        rc.udp_bind_port = cfg->udp_bind_port;
        rc.udp_target_port = cfg->udp_target_port;
        rc.tcp_listen_port = cfg->tcp_listen_port;
        rc.connect_timeout_ms = cfg->connect_timeout_ms;
        rc.io_poll_interval_ms = cfg->io_poll_interval_ms;
        rc.max_buffer_bytes_per_peer = cfg->max_buffer_bytes_per_peer;
        rc.self_identity.agent_type =
            static_cast<sunraycom::AgentType>(cfg->self_identity.agent_type);
        rc.self_identity.agent_id = cfg->self_identity.agent_id;
        rc.self_identity.role = static_cast<sunraycom::EndpointRole>(cfg->self_identity.role);
        rc.capability_flags = cfg->capability_flags;
        if (cfg->shared_secret[0] != '\0') {
            rc.shared_secret = cfg->shared_secret;
        }
    }

    auto& bus = handle->runtime.event_bus();
    handle->tok_envelope = bus.subscribe_envelope([handle](const sunraycom::EnvelopeEvent& ev) {
        sunraycom_event_t out{};
        out.type = SUNRAYCOM_EVENT_ENVELOPE;
        out.transport = static_cast<uint8_t>(ev.transport);
        out.protocol_major = ev.envelope.protocol_major;
        out.qos_class = static_cast<uint8_t>(ev.envelope.qos_class);
        out.message_family = static_cast<uint8_t>(ev.envelope.message_family);
        out.message_type = ev.envelope.message_type;
        out.session_id = ev.envelope.session_id;
        out.message_id = ev.envelope.message_id;
        out.correlation_id = ev.envelope.correlation_id;
        out.created_at_ms = ev.envelope.created_at_ms;
        out.ttl_ms = ev.envelope.ttl_ms;
        out.checksum = ev.envelope.checksum;
        out.source_type = static_cast<uint8_t>(ev.envelope.source.agent_type);
        out.source_id = ev.envelope.source.agent_id;
        out.source_role = static_cast<uint8_t>(ev.envelope.source.role);
        out.target_scope = static_cast<uint8_t>(ev.envelope.target.scope);
        out.target_type = static_cast<uint8_t>(ev.envelope.target.target_type);
        out.target_group_id = ev.envelope.target.group_id;
        out.peer_port = ev.peer.port;
        safe_copy(out.peer_id, sizeof(out.peer_id), ev.peer.id);
        safe_copy(out.peer_ip, sizeof(out.peer_ip), ev.peer.ip);
        out.payload_len = std::min(sizeof(out.payload), ev.envelope.payload.size());
        if (out.payload_len > 0) {
            std::memcpy(out.payload, ev.envelope.payload.data(), out.payload_len);
        }
        std::lock_guard<std::mutex> lock(handle->mu);
        handle->queue.push_back(out);
    });

    handle->tok_error = bus.subscribe_error([handle](const sunraycom::ErrorEvent& ev) {
        sunraycom_event_t out{};
        out.type = SUNRAYCOM_EVENT_ERROR;
        out.transport = static_cast<uint8_t>(ev.transport);
        out.code = static_cast<uint16_t>(ev.code);
        out.peer_port = ev.peer.port;
        safe_copy(out.peer_id, sizeof(out.peer_id), ev.peer.id);
        safe_copy(out.peer_ip, sizeof(out.peer_ip), ev.peer.ip);
        safe_copy(out.message, sizeof(out.message), ev.message);
        std::lock_guard<std::mutex> lock(handle->mu);
        handle->queue.push_back(out);
    });

    handle->tok_link = bus.subscribe_link([handle](const sunraycom::LinkEvent& ev) {
        sunraycom_event_t out{};
        out.type = SUNRAYCOM_EVENT_LINK;
        out.transport = static_cast<uint8_t>(ev.transport);
        out.peer_port = ev.peer.port;
        out.link_up = ev.is_up ? 1 : 0;
        safe_copy(out.peer_id, sizeof(out.peer_id), ev.peer.id);
        safe_copy(out.peer_ip, sizeof(out.peer_ip), ev.peer.ip);
        std::lock_guard<std::mutex> lock(handle->mu);
        handle->queue.push_back(out);
    });

    const auto ec = handle->runtime.start(rc);
    return ec == sunraycom::ErrorCode::kOk ? SUNRAYCOM_STATUS_OK : SUNRAYCOM_STATUS_ERR;
}

void sunraycom_stop(sunraycom_handle_t* handle) {
    if (!handle) {
        return;
    }
    handle->runtime.stop();
}

int sunraycom_send_udp_unicast(sunraycom_handle_t* handle,
                               const uint8_t* data,
                               size_t len,
                               const char* ip,
                               uint16_t port) {
    if (!handle || !ip) {
        return -1;
    }
    return handle->runtime.udp().send_unicast(to_bytes(data, len), ip, port);
}

int sunraycom_send_udp_broadcast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port) {
    if (!handle) {
        return -1;
    }
    return handle->runtime.udp().send_broadcast(to_bytes(data, len), port);
}

int sunraycom_send_udp_multicast(sunraycom_handle_t* handle,
                                 const uint8_t* data,
                                 size_t len,
                                 uint16_t port) {
    if (!handle) {
        return -1;
    }
    return handle->runtime.udp().send_multicast(to_bytes(data, len), port);
}

sunraycom_status_t sunraycom_tcp_connect(sunraycom_handle_t* handle,
                                         const char* ip,
                                         uint16_t port,
                                         char* out_peer_id,
                                         size_t out_peer_id_len) {
    if (!handle || !ip) {
        return SUNRAYCOM_STATUS_ERR;
    }
    std::string peer_id;
    const auto ec = handle->runtime.tcp_clients().connect_peer(ip, port, &peer_id);
    if (ec != sunraycom::ErrorCode::kOk) {
        return SUNRAYCOM_STATUS_ERR;
    }
    if (out_peer_id && out_peer_id_len > 0) {
        safe_copy(out_peer_id, out_peer_id_len, peer_id);
    }
    return SUNRAYCOM_STATUS_OK;
}

int sunraycom_send_tcp(sunraycom_handle_t* handle,
                       const char* peer_id,
                       const uint8_t* data,
                       size_t len) {
    if (!handle || !peer_id) {
        return -1;
    }
    return handle->runtime.tcp_clients().send(peer_id, to_bytes(data, len));
}

sunraycom_status_t sunraycom_poll_event(sunraycom_handle_t* handle, sunraycom_event_t* out_event) {
    if (!handle || !out_event) {
        return SUNRAYCOM_STATUS_ERR;
    }
    std::lock_guard<std::mutex> lock(handle->mu);
    if (handle->queue.empty()) {
        std::memset(out_event, 0, sizeof(*out_event));
        out_event->type = SUNRAYCOM_EVENT_NONE;
        return SUNRAYCOM_STATUS_OK;
    }
    *out_event = handle->queue.front();
    handle->queue.pop_front();
    return SUNRAYCOM_STATUS_OK;
}

}  // extern "C"
