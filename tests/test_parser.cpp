/**
 * @file tests/test_parser.cpp
 * @brief sunray_communication_lib source file.
 */

#include <iostream>

#include "sunraycom/core/envelope_stream_parser.hpp"
#include "sunraycom/core/semantic_messages.hpp"

int main() {
    sunraycom::ProtocolCodec codec;
    const sunraycom::EndpointIdentity source{
        sunraycom::AgentType::kGroundStation,
        11,
        sunraycom::EndpointRole::kController,
    };

    sunraycom::TakeoffCommand takeoff{};
    takeoff.relative_height_m = 12.5F;
    takeoff.max_velocity_mps = 3.2F;
    const auto a = sunraycom::make_typed_envelope(
        source,
        sunraycom::TargetSelector::for_entity(sunraycom::AgentType::kUav, 1),
        7001,
        8001,
        sunraycom::QosClass::kReliableOrdered,
        takeoff,
        200);

    sunraycom::VehicleEvent vehicle_event{};
    vehicle_event.kind = sunraycom::VehicleEventKind::kTakeoff;
    vehicle_event.severity = 2;
    vehicle_event.detail = "armed";
    const auto b = sunraycom::make_typed_envelope(
        source,
        sunraycom::TargetSelector::broadcast(sunraycom::AgentType::kGroundStation),
        0,
        8002,
        sunraycom::QosClass::kBestEffort,
        vehicle_event);

    const auto ba = codec.encode(a, true);
    const auto bb = codec.encode(b, true);

    sunraycom::EnvelopeStreamParser parser;
    parser.feed(ba.data(), 5);
    parser.feed(ba.data() + 5, ba.size() - 5);
    parser.feed(bb);

    sunraycom::SecureEnvelope out;
    if (!parser.pop_next(&out) || out.message_id != a.message_id ||
        out.message_family != sunraycom::MessageFamily::kCommand) {
        std::cerr << "first envelope failed\n";
        return 1;
    }

    if (!parser.pop_next(&out) || out.message_id != b.message_id ||
        out.message_family != sunraycom::MessageFamily::kStateEvent ||
        out.target.scope != sunraycom::TargetScope::kBroadcast) {
        std::cerr << "second envelope failed\n";
        return 2;
    }
    return 0;
}
