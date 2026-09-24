#include <cassert>
#include <algorithm>

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
    assert(may_drop_congested_frame(QosClass::kBestEffort, 0));
    assert(may_drop_congested_frame(QosClass::kBulk, 0));
    assert(!may_drop_congested_frame(QosClass::kBestEffort, 1));
    assert(!may_drop_congested_frame(QosClass::kBulk, 8192));
    assert(!may_drop_congested_frame(QosClass::kReliableOrdered, 0));

    const auto lossy = encode_frame(QosClass::kBestEffort, 64);
    const auto control = encode_frame(QosClass::kReliableOrdered, 64);
    assert(!lossy.empty());
    assert(!control.empty());
    Bytes written;
    int writes = 0;
    auto partial = write_frame_chunks(
        lossy,
        [&](const uint8_t* data, size_t size, std::error_code& error) {
            if (writes++ == 0) {
                const auto count = std::min(size, size_t{12});
                written.insert(written.end(), data, data + count);
                return count;
            }
            error = asio::error::would_block;
            return size_t{0};
        },
        [] { return true; },
        2);
    assert(partial.status == SocketWriteStatus::kCongested);
    assert(partial.bytes_written == written.size() && !written.empty());
    assert(!may_drop_congested_frame(QosClass::kBestEffort, partial.bytes_written));
    // A short congestion that clears must finish the original frame before
    // another frame is eligible to write on this TCP stream.
    written.clear();
    writes = 0;
    const auto recovered = write_frame_chunks(
        lossy,
        [&](const uint8_t* data, size_t size, std::error_code& error) {
            const int attempt = writes++;
            if (attempt == 0) {
                written.insert(written.end(), data, data + 12);
                return size_t{12};
            }
            if (attempt == 1) {
                // Simulate temporary backpressure on the second chunk.
                error = asio::error::would_block;
                return size_t{0};
            }
            written.insert(written.end(), data, data + size);
            return size;
        },
        [] { return true; },
        2);
    assert(recovered.status == SocketWriteStatus::kComplete);
    assert(written == lossy);
    const auto complete = write_frame_chunks(
        control,
        [&](const uint8_t* data, size_t size, std::error_code&) {
            written.insert(written.end(), data, data + size);
            return size;
        },
        [] { return true; },
        2);
    assert(complete.status == SocketWriteStatus::kComplete);
    assert(complete.bytes_written == control.size());
    const auto first = WireCodec().decode(written.data(), written.size(), 1);
    assert(first.ok() && first.consumed == lossy.size());
    const auto second = WireCodec().decode(written.data() + first.consumed,
                                           written.size() - first.consumed, 1);
    assert(second.ok() && second.consumed == control.size());

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
