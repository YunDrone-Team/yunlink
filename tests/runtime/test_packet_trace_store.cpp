/**
 * @file tests/runtime/test_packet_trace_store.cpp
 * @brief Packet trace store memory bound and preview truncation tests.
 */

#include <iostream>

#include "yunlink/diagnostics/packet_trace.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace {

yunlink::SecureEnvelope make_envelope(size_t payload_size) {
    yunlink::SecureEnvelope envelope;
    envelope.message_family = yunlink::MessageFamily::kCommand;
    envelope.message_type = static_cast<uint16_t>(yunlink::CommandType::kTakeoff);
    envelope.session_id = 10;
    envelope.message_id = 20;
    envelope.correlation_id = 30;
    envelope.header_len = yunlink::ProtocolCodec::kFixedHeaderSize;
    envelope.payload_len = static_cast<uint32_t>(payload_size);
    envelope.checksum = 1234;
    envelope.payload.assign(payload_size, 0x42);
    return envelope;
}

yunlink::PacketTraceRecord make_record(size_t raw_size, size_t payload_size) {
    const auto envelope = make_envelope(payload_size);
    yunlink::ByteBuffer raw(raw_size, 0xAA);
    yunlink::PeerInfo peer;
    peer.id = "127.0.0.1:10000";
    return yunlink::make_packet_trace_record(yunlink::PacketTraceDirection::kRx,
                                             yunlink::PacketTraceStage::kDecodeSucceeded,
                                             yunlink::TransportType::kUdpUnicast,
                                             peer,
                                             &envelope,
                                             raw.data(),
                                             raw.size(),
                                             yunlink::ErrorCode::kOk,
                                             "",
                                             8,
                                             6);
}

bool verify_canonical_wire_trace() {
    auto envelope = make_envelope(12);
    envelope.header_len = yunlink::ProtocolCodec::kFixedHeaderSize;
    envelope.checksum = 0;
    envelope.security.auth_tag.assign(24, 0x7B);
    envelope.target = yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1);
    envelope.target.target_ids = {1, 2, 3};

    const auto encoded = yunlink::ProtocolCodec().encode(envelope);
    const auto record =
        yunlink::make_packet_trace_record(yunlink::PacketTraceDirection::kTx,
                                          yunlink::PacketTraceStage::kEncodedForSend,
                                          yunlink::TransportType::kTcpClient,
                                          {},
                                          &envelope,
                                          encoded.data(),
                                          encoded.size());
    const uint16_t expected_header_len = static_cast<uint16_t>(
        yunlink::ProtocolCodec::kFixedHeaderSize +
        envelope.target.target_ids.size() * sizeof(uint32_t) + envelope.security.auth_tag.size());
    if (record.header_len != expected_header_len ||
        record.envelope.header_len != expected_header_len) {
        std::cerr << "wire canonical header_len mismatch\n";
        return false;
    }
    if (record.checksum == 0 || record.total_len != encoded.size()) {
        std::cerr << "wire canonical checksum/total_len mismatch\n";
        return false;
    }
    if (record.envelope.security.auth_tag.size() != envelope.security.auth_tag.size()) {
        std::cerr << "wire canonical auth_tag mismatch\n";
        return false;
    }
    return true;
}

}  // namespace

int main() {
    if (!verify_canonical_wire_trace()) {
        return 1;
    }

    yunlink::PacketTraceStoreConfig config;
    config.enabled = true;
    config.max_records = 3;
    config.max_total_bytes = yunlink::estimated_packet_trace_bytes(make_record(64, 32)) * 2;
    config.raw_preview_bytes = 8;
    config.payload_preview_bytes = 6;

    yunlink::PacketTraceStore store(config);
    for (int i = 0; i < 5; ++i) {
        store.push(make_record(64, 32));
    }

    const auto records = store.snapshot();
    if (records.size() != 2) {
        std::cerr << "byte budget should retain two records, got " << records.size() << "\n";
        return 2;
    }
    if (store.total_bytes() > config.max_total_bytes) {
        std::cerr << "trace byte budget exceeded\n";
        return 3;
    }
    for (const auto& record : records) {
        if (record.raw_preview.size() != config.raw_preview_bytes ||
            record.payload_preview.size() != config.payload_preview_bytes) {
            std::cerr << "preview limit not applied\n";
            return 4;
        }
        if (!record.raw_truncated || !record.payload_truncated) {
            std::cerr << "preview truncation flags not set\n";
            return 5;
        }
        if (!record.envelope.payload.empty()) {
            std::cerr << "stored trace retained full payload\n";
            return 6;
        }
    }

    yunlink::PacketTraceStoreConfig count_config = config;
    count_config.max_total_bytes = yunlink::estimated_packet_trace_bytes(make_record(1, 1)) * 10;
    yunlink::PacketTraceStore count_store(count_config);
    for (int i = 0; i < 5; ++i) {
        count_store.push(make_record(1, 1));
    }
    if (count_store.snapshot().size() != count_config.max_records) {
        std::cerr << "record limit not enforced\n";
        return 7;
    }
    return 0;
}
