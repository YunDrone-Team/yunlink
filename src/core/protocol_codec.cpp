/**
 * @file src/core/protocol_codec.cpp
 * @brief SunrayComLib source file.
 */

#include "sunraycom/core/protocol_codec.hpp"

#include <chrono>

namespace sunraycom {

namespace {

uint64_t now_millis() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

}  // namespace

bool ProtocolCodec::is_valid_head(uint16_t head) {
    return head == kHeadTcp || head == kHeadUdpState || head == kHeadUdpSearch ||
           head == kHeadUdpAck || head == kHeadShared;
}

uint16_t ProtocolCodec::infer_head(uint8_t seq) {
    switch (static_cast<MessageId>(seq)) {
    case MessageId::HeartbeatMessageID:
    case MessageId::UAVControlCMDMessageID:
    case MessageId::UGVControlCMDMessageID:
    case MessageId::UAVSetupMessageID:
    case MessageId::DemoMessageID:
    case MessageId::ScriptMessageID:
    case MessageId::WaypointMessageID:
    case MessageId::ViobotSwitchMessageID:
    case MessageId::RTKOriginMessageID:
    case MessageId::PointCloudDataSwitchMessageID:
        return kHeadTcp;
    case MessageId::UAVStateMessageID:
    case MessageId::UGVStateMessageID:
    case MessageId::NodeMessageID:
    case MessageId::AgentComputerStatusMessageID:
    case MessageId::FACMapDataMessageID:
    case MessageId::FACCompetitionStateMessageID:
    case MessageId::WaypointStateMessageID:
    case MessageId::PX4StateMessageID:
    case MessageId::PX4ParameterMessageID:
    case MessageId::QRCodeCoordMessageID:
    case MessageId::PointCloudDataMessageID:
        return kHeadUdpState;
    case MessageId::SearchMessageID:
        return kHeadUdpSearch;
    case MessageId::ACKMessageID:
        return kHeadUdpAck;
    case MessageId::FormationMessageID:
    case MessageId::GroundFormationMessageID:
    case MessageId::GoalMessageID:
        return kHeadShared;
    default:
        return 0;
    }
}

uint16_t ProtocolCodec::checksum(const uint8_t* data, size_t len) {
    uint32_t sum = 0;
    for (size_t i = 0; i < len; ++i) {
        sum += static_cast<uint8_t>(data[i]);
    }
    return static_cast<uint16_t>(sum & 0xFFFFU);
}

ByteBuffer ProtocolCodec::encode(const Frame& frame, bool auto_fill_header) const {
    Frame working = frame;
    if (auto_fill_header) {
        if (working.header.head == 0) {
            working.header.head = infer_head(working.header.seq);
        }
        working.header.length = static_cast<uint32_t>(working.payload.size() + kMinFrameSize);
        if (working.header.timestamp == 0) {
            working.header.timestamp = now_millis();
        }
    }
    if (!is_valid_head(working.header.head) || working.header.length < kMinFrameSize) {
        return {};
    }

    ByteBuffer out;
    out.reserve(working.payload.size() + kMinFrameSize);
    // Head is serialized as high byte then low byte in legacy implementation.
    out.push_back(static_cast<uint8_t>((working.header.head >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(working.header.head & 0xFF));

    for (int i = 0; i < 4; ++i) {
        out.push_back(static_cast<uint8_t>((working.header.length >> (i * 8)) & 0xFF));
    }

    out.push_back(working.header.seq);
    out.push_back(working.header.robot_id);

    for (int i = 0; i < 8; ++i) {
        out.push_back(static_cast<uint8_t>((working.header.timestamp >> (i * 8)) & 0xFF));
    }

    out.insert(out.end(), working.payload.begin(), working.payload.end());

    working.header.checksum = checksum(out.data(), out.size());
    // Legacy wire format places high byte first then low byte.
    out.push_back(static_cast<uint8_t>((working.header.checksum >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>(working.header.checksum & 0xFF));
    return out;
}

DecodeResult ProtocolCodec::decode(const uint8_t* data, size_t len) const {
    DecodeResult result;
    if (data == nullptr || len < kMinFrameSize) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }

    const uint16_t head = static_cast<uint16_t>(data[1]) | (static_cast<uint16_t>(data[0]) << 8);
    if (!is_valid_head(head)) {
        result.code = ErrorCode::kInvalidHeader;
        return result;
    }

    uint32_t length = 0;
    for (int i = 0; i < 4; ++i) {
        length |= static_cast<uint32_t>(static_cast<uint8_t>(data[2 + i])) << (i * 8);
    }

    if (length < kMinFrameSize || length > len) {
        result.code = ErrorCode::kDecodeError;
        return result;
    }

    const uint16_t wire_checksum =
        static_cast<uint16_t>(data[length - 1]) | (static_cast<uint16_t>(data[length - 2]) << 8);
    const uint16_t calc_checksum = checksum(data, length - 2);
    if (wire_checksum != calc_checksum) {
        result.code = ErrorCode::kChecksumMismatch;
        result.consumed = length;
        return result;
    }

    Frame frame;
    frame.header.head = head;
    frame.header.length = length;
    frame.header.seq = data[6];
    frame.header.robot_id = data[7];
    frame.header.timestamp = 0;
    for (int i = 0; i < 8; ++i) {
        frame.header.timestamp |= static_cast<uint64_t>(static_cast<uint8_t>(data[8 + i]))
                                  << (i * 8);
    }
    frame.header.checksum = wire_checksum;

    const size_t payload_len = static_cast<size_t>(length) - kMinFrameSize;
    frame.payload.assign(data + 16, data + 16 + payload_len);

    result.code = ErrorCode::kOk;
    result.consumed = length;
    result.frame = std::move(frame);
    return result;
}

}  // namespace sunraycom
