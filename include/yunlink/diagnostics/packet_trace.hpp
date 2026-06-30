/**
 * @file include/yunlink/diagnostics/packet_trace.hpp
 * @brief YunLink runtime packet trace records and bounded storage.
 */

#ifndef YUNLINK_CORE_PACKET_TRACE_HPP
#define YUNLINK_CORE_PACKET_TRACE_HPP

#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

#include "yunlink/core/types.hpp"

namespace yunlink {

enum class PacketTraceDirection : uint8_t {
    kRx = 1,
    kTx = 2,
};

enum class PacketTraceStage : uint8_t {
    kRawReceived = 1,
    kDecodeSucceeded = 2,
    kDecodeFailed = 3,
    kDispatchAccepted = 4,
    kDispatchRejected = 5,
    kEncodedForSend = 6,
    kSendSucceeded = 7,
    kSendFailed = 8,
};

struct PacketTraceStoreConfig {
    bool enabled = false;
    size_t max_records = 500;
    size_t max_total_bytes = 8 * 1024 * 1024;
    size_t raw_preview_bytes = 4096;
    size_t payload_preview_bytes = 4096;
};

struct PacketTraceRecord {
    uint64_t trace_id = 0;
    uint64_t observed_at_ms = 0;
    PacketTraceDirection direction = PacketTraceDirection::kRx;
    PacketTraceStage stage = PacketTraceStage::kRawReceived;
    TransportType transport = TransportType::kUdpUnicast;
    PeerInfo peer;
    bool has_envelope = false;
    ErrorCode code = ErrorCode::kOk;
    std::string error_message;
    uint16_t header_len = 0;
    uint32_t payload_len = 0;
    uint32_t total_len = 0;
    uint32_t checksum = 0;
    SecureEnvelope envelope;
    ByteBuffer raw_preview;
    ByteBuffer payload_preview;
    bool raw_truncated = false;
    bool payload_truncated = false;
    size_t target_ids_total = 0;
    size_t auth_tag_total = 0;
    bool metadata_truncated = false;
};

class PacketTraceStore {
  public:
    PacketTraceStore();
    explicit PacketTraceStore(PacketTraceStoreConfig config);

    void configure(PacketTraceStoreConfig config);
    const PacketTraceStoreConfig& config() const;
    bool enabled() const;
    bool push(PacketTraceRecord record, PacketTraceRecord* stored = nullptr);
    std::vector<PacketTraceRecord> snapshot() const;
    void clear();
    size_t size() const;
    size_t total_bytes() const;

  private:
    PacketTraceStoreConfig config_;
    std::deque<PacketTraceRecord> records_;
    size_t total_bytes_ = 0;
    uint64_t next_trace_id_ = 1;

    void trim_to_limits();
    PacketTraceRecord sanitize(PacketTraceRecord record) const;
    static size_t record_bytes(const PacketTraceRecord& record);
};

size_t estimated_packet_trace_bytes(const PacketTraceRecord& record);

PacketTraceRecord
make_packet_trace_record(PacketTraceDirection direction,
                         PacketTraceStage stage,
                         TransportType transport,
                         const PeerInfo& peer,
                         const SecureEnvelope* envelope,
                         const uint8_t* raw_data,
                         size_t raw_len,
                         ErrorCode code = ErrorCode::kOk,
                         const std::string& error_message = std::string(),
                         size_t raw_preview_limit = std::numeric_limits<size_t>::max(),
                         size_t payload_preview_limit = std::numeric_limits<size_t>::max());

}  // namespace yunlink

#endif  // YUNLINK_CORE_PACKET_TRACE_HPP
