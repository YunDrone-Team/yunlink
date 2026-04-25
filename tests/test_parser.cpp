/**
 * @file tests/test_parser.cpp
 * @brief yunlink source file.
 */

#include <iostream>

#include "yunlink/core/envelope_stream_parser.hpp"
#include "yunlink/core/semantic_messages.hpp"

int main() {
    yunlink::ProtocolCodec codec;
    const yunlink::EndpointIdentity source{
        yunlink::AgentType::kGroundStation,
        11,
        yunlink::EndpointRole::kController,
    };

    yunlink::TakeoffCommand takeoff{};
    takeoff.relative_height_m = 12.5F;
    takeoff.max_velocity_mps = 3.2F;
    const auto a = yunlink::make_typed_envelope(
        source,
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, 1),
        7001,
        8001,
        yunlink::QosClass::kReliableOrdered,
        takeoff,
        200);

    yunlink::VehicleEvent vehicle_event{};
    vehicle_event.kind = yunlink::VehicleEventKind::kTakeoff;
    vehicle_event.severity = 2;
    vehicle_event.detail = "armed";
    const auto b = yunlink::make_typed_envelope(
        source,
        yunlink::TargetSelector::broadcast(yunlink::AgentType::kGroundStation),
        0,
        8002,
        yunlink::QosClass::kBestEffort,
        vehicle_event);

    const auto ba = codec.encode(a, true);
    const auto bb = codec.encode(b, true);

    yunlink::EnvelopeStreamParser parser;
    parser.feed(ba.data(), 5);
    parser.feed(ba.data() + 5, ba.size() - 5);
    parser.feed(bb);

    yunlink::SecureEnvelope out;
    if (!parser.pop_next(&out) || out.message_id != a.message_id ||
        out.message_family != yunlink::MessageFamily::kCommand) {
        std::cerr << "first envelope failed\n";
        return 1;
    }

    if (!parser.pop_next(&out) || out.message_id != b.message_id ||
        out.message_family != yunlink::MessageFamily::kStateEvent ||
        out.target.scope != yunlink::TargetScope::kBroadcast) {
        std::cerr << "second envelope failed\n";
        return 2;
    }
    return 0;
}
