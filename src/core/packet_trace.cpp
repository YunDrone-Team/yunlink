/**
 * @file src/core/packet_trace.cpp
 * @brief YunLink runtime packet trace storage.
 */

#include "yunlink/diagnostics/packet_trace.hpp"

#include <algorithm>
#include <chrono>
#include <utility>

#include "yunlink/core/protocol_codec.hpp"

namespace yunlink {

namespace {

constexpr size_t kMaxPeerFieldBytes = 128;
constexpr size_t kMaxErrorMessageBytes = 512;
constexpr size_t kMaxTraceTargetIds = 32;
constexpr size_t kMaxTraceAuthTagBytes = 32;

uint64_t trace_now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

ByteBuffer preview_bytes(const uint8_t* data, size_t len, size_t limit, bool* truncated) {
    ByteBuffer out;
    if (data == nullptr || len == 0 || limit == 0) {
        if (truncated != nullptr) {
            *truncated = len > 0;
        }
        return out;
    }
    const size_t keep = std::min(len, limit);
    out.assign(data, data + keep);
    if (truncated != nullptr) {
        *truncated = len > keep;
    }
    return out;
}

SecureEnvelope envelope_metadata(const SecureEnvelope& envelope) {
    SecureEnvelope out;
    out.protocol_major = envelope.protocol_major;
    out.header_version = envelope.header_version;
    out.flags = envelope.flags;
    out.header_len = envelope.header_len;
    out.payload_len = envelope.payload_len;
    out.qos_class = envelope.qos_class;
    out.message_family = envelope.message_family;
    out.message_type = envelope.message_type;
    out.schema_version = envelope.schema_version;
    out.session_id = envelope.session_id;
    out.message_id = envelope.message_id;
    out.correlation_id = envelope.correlation_id;
    out.source = envelope.source;
    out.target = envelope.target;
    out.created_at_ms = envelope.created_at_ms;
    out.ttl_ms = envelope.ttl_ms;
    out.security = envelope.security;
    out.checksum = envelope.checksum;
    return out;
}

void truncate_string(std::string* value, size_t limit, bool* truncated) {
    if (value == nullptr || value->size() <= limit) {
        return;
    }
    value->resize(limit);
    if (truncated != nullptr) {
        *truncated = true;
    }
}

bool decode_wire_for_trace(const uint8_t* data, size_t len, SecureEnvelope* out) {
    if (data == nullptr || len < ProtocolCodec::kMinEnvelopeSize ||
        !ProtocolCodec::has_magic(data, len)) {
        return false;
    }
    DecodeResult decoded = ProtocolCodec().decode(data, len);
    if (!decoded.ok()) {
        return false;
    }
    if (out != nullptr) {
        *out = std::move(decoded.envelope);
    }
    return true;
}

}  // namespace

PacketTraceStore::PacketTraceStore() = default;

PacketTraceStore::PacketTraceStore(PacketTraceStoreConfig config) : config_(config) {}

void PacketTraceStore::configure(PacketTraceStoreConfig config) {
    config_ = config;
    clear();
}

const PacketTraceStoreConfig& PacketTraceStore::config() const {
    return config_;
}

bool PacketTraceStore::enabled() const {
    return config_.enabled && config_.max_records > 0 && config_.max_total_bytes > 0;
}

bool PacketTraceStore::push(PacketTraceRecord record, PacketTraceRecord* stored) {
    if (!enabled()) {
        return false;
    }

    PacketTraceRecord sanitized = sanitize(std::move(record));
    if (sanitized.trace_id == 0) {
        sanitized.trace_id = next_trace_id_++;
    } else {
        next_trace_id_ = std::max(next_trace_id_, sanitized.trace_id + 1);
    }
    if (sanitized.observed_at_ms == 0) {
        sanitized.observed_at_ms = trace_now_millis();
    }

    const size_t bytes = record_bytes(sanitized);
    records_.push_back(std::move(sanitized));
    total_bytes_ += bytes;
    trim_to_limits();

    if (stored != nullptr && !records_.empty()) {
        *stored = records_.back();
    }
    return true;
}

std::vector<PacketTraceRecord> PacketTraceStore::snapshot() const {
    return std::vector<PacketTraceRecord>(records_.begin(), records_.end());
}

void PacketTraceStore::clear() {
    records_.clear();
    total_bytes_ = 0;
}

size_t PacketTraceStore::size() const {
    return records_.size();
}

size_t PacketTraceStore::total_bytes() const {
    return total_bytes_;
}

void PacketTraceStore::trim_to_limits() {
    while (!records_.empty() &&
           (records_.size() > config_.max_records || total_bytes_ > config_.max_total_bytes)) {
        total_bytes_ -= record_bytes(records_.front());
        records_.pop_front();
    }
}

PacketTraceRecord PacketTraceStore::sanitize(PacketTraceRecord record) const {
    if (record.raw_preview.size() > config_.raw_preview_bytes) {
        record.raw_preview.resize(config_.raw_preview_bytes);
        record.raw_truncated = true;
    }
    if (record.payload_preview.size() > config_.payload_preview_bytes) {
        record.payload_preview.resize(config_.payload_preview_bytes);
        record.payload_truncated = true;
    }

    truncate_string(&record.peer.id, kMaxPeerFieldBytes, &record.metadata_truncated);
    truncate_string(&record.peer.ip, kMaxPeerFieldBytes, &record.metadata_truncated);
    truncate_string(&record.error_message, kMaxErrorMessageBytes, &record.metadata_truncated);

    record.target_ids_total = record.envelope.target.target_ids.size();
    if (record.envelope.target.target_ids.size() > kMaxTraceTargetIds) {
        record.envelope.target.target_ids.resize(kMaxTraceTargetIds);
        record.metadata_truncated = true;
    }
    record.auth_tag_total = record.envelope.security.auth_tag.size();
    if (record.envelope.security.auth_tag.size() > kMaxTraceAuthTagBytes) {
        record.envelope.security.auth_tag.resize(kMaxTraceAuthTagBytes);
        record.metadata_truncated = true;
    }

    while (record_bytes(record) > config_.max_total_bytes &&
           (!record.raw_preview.empty() || !record.payload_preview.empty())) {
        if (record.payload_preview.size() >= record.raw_preview.size() &&
            !record.payload_preview.empty()) {
            record.payload_preview.pop_back();
            record.payload_truncated = true;
        } else if (!record.raw_preview.empty()) {
            record.raw_preview.pop_back();
            record.raw_truncated = true;
        }
    }

    record.envelope.payload.clear();
    return record;
}

size_t PacketTraceStore::record_bytes(const PacketTraceRecord& record) {
    return estimated_packet_trace_bytes(record);
}

size_t estimated_packet_trace_bytes(const PacketTraceRecord& record) {
    return sizeof(PacketTraceRecord) + record.raw_preview.size() + record.payload_preview.size() +
           record.error_message.size() + record.peer.id.size() + record.peer.ip.size() +
           record.envelope.target.target_ids.size() * sizeof(uint32_t) +
           record.envelope.security.auth_tag.size();
}

PacketTraceRecord make_packet_trace_record(PacketTraceDirection direction,
                                           PacketTraceStage stage,
                                           TransportType transport,
                                           const PeerInfo& peer,
                                           const SecureEnvelope* envelope,
                                           const uint8_t* raw_data,
                                           size_t raw_len,
                                           ErrorCode code,
                                           const std::string& error_message,
                                           size_t raw_preview_limit,
                                           size_t payload_preview_limit) {
    PacketTraceRecord record;
    record.direction = direction;
    record.stage = stage;
    record.transport = transport;
    record.peer = peer;
    record.code = code;
    record.error_message = error_message;
    record.total_len = static_cast<uint32_t>(std::min<size_t>(raw_len, UINT32_MAX));
    record.raw_preview = preview_bytes(raw_data, raw_len, raw_preview_limit, &record.raw_truncated);

    SecureEnvelope canonical;
    const SecureEnvelope* source = envelope;
    if (decode_wire_for_trace(raw_data, raw_len, &canonical)) {
        source = &canonical;
    }

    if (source != nullptr) {
        record.has_envelope = true;
        record.envelope = envelope_metadata(*source);
        record.header_len = source->header_len;
        record.payload_len = source->payload_len;
        record.checksum = source->checksum;
        record.total_len = static_cast<uint32_t>(
            std::min<size_t>(static_cast<size_t>(source->header_len) + source->payload.size() +
                                 ProtocolCodec::kTrailerSize,
                             UINT32_MAX));
        record.payload_preview = preview_bytes(source->payload.data(),
                                               source->payload.size(),
                                               payload_preview_limit,
                                               &record.payload_truncated);
        record.target_ids_total = source->target.target_ids.size();
        record.auth_tag_total = source->security.auth_tag.size();
    }
    return record;
}

}  // namespace yunlink
