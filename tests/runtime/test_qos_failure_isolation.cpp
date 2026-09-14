#include <cassert>

#include "runtime/v2/runtime_internal.hpp"
#include "yunlink/core/wire_v2_codec.hpp"

namespace {

yunlink::v2::Bytes encode_frame(yunlink::v2::QosClass qos, size_t payload_bytes) {
    yunlink::v2::Envelope envelope;
    envelope.family = yunlink::v2::MessageFamily::kStream;
    envelope.operation = 4;
    envelope.qos_class = qos;
    envelope.session_id = 1;
    envelope.message_id = 1;
    envelope.source = {"endpoint.a", "entity.a"};
    envelope.target = yunlink::v2::TargetSelector::broadcast();
    envelope.type = {"org.yunlink.visual", 1, 0, "Sample"};
    envelope.created_at_ms = 1;
    envelope.ttl_ms = 1000;
    envelope.payload.assign(payload_bytes, 0x11);
    return yunlink::v2::WireCodec().encode(envelope);
}

}  // namespace

int main() {
    using namespace yunlink::v2;

    assert(qos_may_drop(QosClass::kBestEffort));
    assert(qos_may_drop(QosClass::kBulk));
    assert(!qos_may_drop(QosClass::kReliableOrdered));
    assert(!qos_may_drop(QosClass::kReliableLatest));

    const auto lossy = encode_frame(QosClass::kBestEffort, 64);
    const auto control = encode_frame(QosClass::kReliableOrdered, 64);
    assert(!lossy.empty());
    assert(!control.empty());

    Bytes overflow = lossy;
    overflow.insert(overflow.end(), 256, 0x22);
    assert(recover_receive_overflow(&overflow, 80));
    assert(overflow.size() < lossy.size() + 256);

    Bytes control_overflow = control;
    control_overflow.insert(control_overflow.end(), 256, 0x22);
    assert(!recover_receive_overflow(&control_overflow, 80));

    Bytes under = lossy;
    assert(recover_receive_overflow(&under, under.size() + 1));
    assert(under == lossy);
    return 0;
}
