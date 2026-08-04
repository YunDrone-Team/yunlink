#include <cassert>
#include <cstdint>
#include <string>

#include "yunlink/core/wire_v2_codec.hpp"

int main() {
    using namespace yunlink::v2;

    assert(valid_uid("e-endpoint.uav-1"));
    assert(!valid_uid("bad uid"));
    assert(!valid_uid(std::string(129, 'x')));

    Envelope envelope;
    envelope.family = MessageFamily::kStream;
    envelope.operation = 4;
    envelope.qos_class = QosClass::kReliableLatest;
    envelope.session_id = 42;
    envelope.message_id = 99;
    envelope.source = {"bridge.alpha", "e-bridge.alpha-uav-1"};
    envelope.target = TargetSelector::entity("gcs.operator");
    envelope.type = {"com.yundrone.sunray", 2, 0, "FlightControlState"};
    envelope.created_at_ms = 1000;
    envelope.ttl_ms = 250;
    envelope.payload = {0x08, 0x01};
    envelope.security = {7, {1, 2, 3, 4}};

    WireCodec codec;
    const auto bytes = codec.encode(envelope);
    assert(!bytes.empty());
    const auto decoded = codec.decode(bytes.data(), bytes.size(), 1100);
    assert(decoded.ok());
    assert(decoded.consumed == bytes.size());
    assert(decoded.envelope.protocol_major == 2);
    assert(decoded.envelope.header_version == 2);
    assert(decoded.envelope.schema_version == 2);
    assert(decoded.envelope.source.entity_uid == envelope.source.entity_uid);
    assert(decoded.envelope.target.uids == envelope.target.uids);
    assert(decoded.envelope.type.profile_id == envelope.type.profile_id);
    assert(decoded.envelope.type.type_name == envelope.type.type_name);
    assert(decoded.envelope.payload == envelope.payload);

    auto corrupt = bytes;
    corrupt[corrupt.size() - 5] ^= 0x80;
    assert(codec.decode(corrupt.data(), corrupt.size()).code == ErrorCode::kChecksumMismatch);
    assert(codec.decode(bytes.data(), bytes.size(), 1300).code == ErrorCode::kTimeout);

    auto v1 = bytes;
    v1[4] = 1;
    assert(codec.decode(v1.data(), v1.size()).code == ErrorCode::kProtocolMismatch);

    envelope.target = TargetSelector::group("sunray.group.7");
    assert(envelope.target.matches("other", {}, {"sunray.group.7"}));
    assert(!envelope.target.matches("other", {}, {"sunray.group.8"}));
    return 0;
}
