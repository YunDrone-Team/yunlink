#include <iostream>
#include <vector>

#include "yunlink/core/envelope_stream_parser.hpp"
#include "yunlink/core/semantic_messages.hpp"

namespace {

yunlink::SecureEnvelope make_goto(uint64_t message_id, uint32_t target_id) {
    yunlink::GotoCommand cmd{};
    cmd.x_m = 1.0F;
    cmd.y_m = 2.0F;
    cmd.z_m = 3.0F;
    cmd.yaw_rad = 0.1F;

    auto envelope = yunlink::make_typed_envelope(
        {yunlink::AgentType::kGroundStation, 9, yunlink::EndpointRole::kController},
        yunlink::TargetSelector::for_entity(yunlink::AgentType::kUav, target_id),
        7000 + target_id,
        0,
        yunlink::QosClass::kReliableOrdered,
        cmd,
        1000);
    envelope.message_id = message_id;
    envelope.correlation_id = message_id;
    return envelope;
}

}  // namespace

int main() {
    yunlink::ProtocolCodec codec;
    const auto first = codec.encode(make_goto(1001, 1), true);
    const auto second = codec.encode(make_goto(1002, 2), true);
    const std::vector<uint8_t> garbage_prefix = {0x01, 0x02, 0x03, 0x04, 0x05};
    const std::vector<uint8_t> garbage_suffix = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};

    if (first.size() < 8 || second.empty()) {
        std::cerr << "fixture encode failed\n";
        return 1;
    }

    yunlink::EnvelopeStreamParser truncated_parser;
    truncated_parser.feed(first.data(), first.size() - 3);
    yunlink::SecureEnvelope out{};
    if (truncated_parser.pop_next(&out)) {
        std::cerr << "truncated frame parsed too early\n";
        return 2;
    }
    truncated_parser.feed(first.data() + first.size() - 3, 3);
    if (!truncated_parser.pop_next(&out) || out.message_id != 1001) {
        std::cerr << "truncated frame did not recover\n";
        return 3;
    }

    yunlink::EnvelopeStreamParser malformed_parser;
    auto malformed = first;
    malformed[8] = 1;
    malformed[9] = 0;
    malformed_parser.feed(malformed);
    malformed_parser.feed(first);
    if (!malformed_parser.pop_next(&out) || out.message_id != 1001) {
        std::cerr << "malformed frame was not skipped\n";
        return 4;
    }

    yunlink::EnvelopeStreamParser garbage_parser;
    garbage_parser.feed(garbage_prefix);
    garbage_parser.feed(first);
    if (!garbage_parser.pop_next(&out) || out.message_id != 1001) {
        std::cerr << "garbage prefix was not skipped\n";
        return 5;
    }

    garbage_parser.feed(garbage_suffix);
    if (garbage_parser.pop_next(&out)) {
        std::cerr << "garbage suffix parsed as envelope\n";
        return 6;
    }

    garbage_parser.feed(second);
    if (!garbage_parser.pop_next(&out) || out.message_id != 1002) {
        std::cerr << "garbage suffix blocked next valid frame\n";
        return 7;
    }

    return 0;
}
