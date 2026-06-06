/**
 * @file include/yunlink/core/semantic_message_codec.hpp
 * @brief 新协议语义消息编解码接口。
 */

#ifndef YUNLINK_CORE_SEMANTIC_MESSAGE_CODEC_HPP
#define YUNLINK_CORE_SEMANTIC_MESSAGE_CODEC_HPP

#include <chrono>

#include "yunlink/core/protocol_codec.hpp"
#include "yunlink/core/semantic/message_traits.hpp"

namespace yunlink {

ByteBuffer encode_payload(const SessionHello& payload);
ByteBuffer encode_payload(const SessionAuthenticate& payload);
ByteBuffer encode_payload(const SessionCapabilities& payload);
ByteBuffer encode_payload(const SessionReady& payload);
ByteBuffer encode_payload(const AuthorityRequest& payload);
ByteBuffer encode_payload(const AuthorityStatus& payload);
ByteBuffer encode_payload(const TakeoffCommand& payload);
ByteBuffer encode_payload(const LandCommand& payload);
ByteBuffer encode_payload(const ReturnCommand& payload);
ByteBuffer encode_payload(const GotoCommand& payload);
ByteBuffer encode_payload(const VelocitySetpointCommand& payload);
ByteBuffer encode_payload(const TrajectoryChunkCommand& payload);
ByteBuffer encode_payload(const FormationTaskCommand& payload);
ByteBuffer encode_payload(const CommandResult& payload);
ByteBuffer encode_payload(const FeatureListRequest& payload);
ByteBuffer encode_payload(const FeatureListResponse& payload);
ByteBuffer encode_payload(const FeatureGetRequest& payload);
ByteBuffer encode_payload(const FeatureGetResponse& payload);
ByteBuffer encode_payload(const FeatureStartRequest& payload);
ByteBuffer encode_payload(const FeatureStartResponse& payload);
ByteBuffer encode_payload(const FeatureStopRequest& payload);
ByteBuffer encode_payload(const FeatureStopResponse& payload);
ByteBuffer encode_payload(const VehicleCoreState& payload);
ByteBuffer encode_payload(const Px4StateSnapshot& payload);
ByteBuffer encode_payload(const OdomStatusSnapshot& payload);
ByteBuffer encode_payload(const UavControlFsmStateSnapshot& payload);
ByteBuffer encode_payload(const UavControllerStateSnapshot& payload);
ByteBuffer encode_payload(const GimbalParamsSnapshot& payload);
ByteBuffer encode_payload(const VehicleEvent& payload);
ByteBuffer encode_payload(const BulkChannelDescriptor& payload);
// 新增
ByteBuffer encode_payload(const LocalOdomSnapshot& payload);
ByteBuffer encode_payload(const UavControlCmdSnapshot& payload);
ByteBuffer encode_payload(const UavControlStateSnapshot& payload);
ByteBuffer encode_payload(const OdomStateSnapshot& payload);

bool decode_payload(const ByteBuffer& bytes, SessionHello* payload);
bool decode_payload(const ByteBuffer& bytes, SessionAuthenticate* payload);
bool decode_payload(const ByteBuffer& bytes, SessionCapabilities* payload);
bool decode_payload(const ByteBuffer& bytes, SessionReady* payload);
bool decode_payload(const ByteBuffer& bytes, AuthorityRequest* payload);
bool decode_payload(const ByteBuffer& bytes, AuthorityStatus* payload);
bool decode_payload(const ByteBuffer& bytes, TakeoffCommand* payload);
bool decode_payload(const ByteBuffer& bytes, LandCommand* payload);
bool decode_payload(const ByteBuffer& bytes, ReturnCommand* payload);
bool decode_payload(const ByteBuffer& bytes, GotoCommand* payload);
bool decode_payload(const ByteBuffer& bytes, VelocitySetpointCommand* payload);
bool decode_payload(const ByteBuffer& bytes, TrajectoryChunkCommand* payload);
bool decode_payload(const ByteBuffer& bytes, FormationTaskCommand* payload);
bool decode_payload(const ByteBuffer& bytes, CommandResult* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureListRequest* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureListResponse* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureGetRequest* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureGetResponse* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureStartRequest* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureStartResponse* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureStopRequest* payload);
bool decode_payload(const ByteBuffer& bytes, FeatureStopResponse* payload);
bool decode_payload(const ByteBuffer& bytes, VehicleCoreState* payload);
bool decode_payload(const ByteBuffer& bytes, Px4StateSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, OdomStatusSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, UavControlFsmStateSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, UavControllerStateSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, GimbalParamsSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, VehicleEvent* payload);
bool decode_payload(const ByteBuffer& bytes, BulkChannelDescriptor* payload);
bool decode_payload(const ByteBuffer& bytes, LocalOdomSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, UavControlCmdSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, UavControlStateSnapshot* payload);
bool decode_payload(const ByteBuffer& bytes, OdomStateSnapshot* payload);

template <typename T> ByteBuffer encode_typed_payload(const T& payload) {
    return encode_payload(payload);
}

template <typename T> bool decode_typed_payload(const ByteBuffer& bytes, T* payload) {
    return decode_payload(bytes, payload);
}

template <typename T>
SecureEnvelope make_typed_envelope(const EndpointIdentity& source,
                                   const TargetSelector& target,
                                   uint64_t session_id,
                                   uint64_t correlation_id,
                                   QosClass qos_class,
                                   const T& payload,
                                   uint32_t ttl_ms = 0) {
    SecureEnvelope envelope;
    envelope.qos_class = qos_class;
    envelope.message_family = MessageTraits<T>::kFamily;
    envelope.message_type = MessageTraits<T>::kMessageType;
    envelope.schema_version = MessageTraits<T>::kSchemaVersion;
    envelope.session_id = session_id;
    envelope.correlation_id = correlation_id;
    envelope.source = source;
    envelope.target = target;
    envelope.ttl_ms = ttl_ms;
    envelope.payload = encode_typed_payload(payload);
    const auto now = std::chrono::system_clock::now();
    envelope.created_at_ms = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
    envelope.message_id = envelope.created_at_ms;
    envelope.payload_len = static_cast<uint32_t>(envelope.payload.size());
    envelope.header_len = static_cast<uint16_t>(
        ProtocolCodec::kFixedHeaderSize + envelope.target.target_ids.size() * sizeof(uint32_t));
    return envelope;
}

}  // namespace yunlink

#endif  // YUNLINK_CORE_SEMANTIC_MESSAGE_CODEC_HPP
