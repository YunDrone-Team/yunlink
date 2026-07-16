/**
 * @file tests/core/test_host_system_snapshot_codec.cpp
 * @brief Host system snapshot schema-1 codec contract.
 */

#include <iostream>

#include "yunlink/core/semantic_message_codec.hpp"

int main() {
    yunlink::HostSystemSnapshot input{};
    input.header.frame_id = "host-system";
    input.header.stamp_ns = 123456789ULL;
    input.cpu_percent = 37.5F;
    input.memory_percent = 62.25F;
    input.sample_period_ms = 1000;
    input.component_kind = "ros1_node";
    input.active_components = {"/rosout", "/sunray_system", "/uav1/mavros"};

    const auto bytes = yunlink::encode_payload(input);
    yunlink::HostSystemSnapshot output{};
    if (bytes.empty() || !yunlink::decode_payload(bytes, &output) ||
        output.header.frame_id != input.header.frame_id ||
        output.header.stamp_ns != input.header.stamp_ns ||
        output.cpu_percent != input.cpu_percent || output.memory_percent != input.memory_percent ||
        output.sample_period_ms != input.sample_period_ms ||
        output.component_kind != input.component_kind ||
        output.active_components != input.active_components) {
        std::cerr << "host system snapshot round-trip failed\n";
        return 1;
    }

    auto truncated = bytes;
    truncated.pop_back();
    if (yunlink::decode_payload(truncated, &output)) {
        std::cerr << "truncated host system snapshot accepted\n";
        return 2;
    }

    input.active_components.assign(513, "/node");
    const auto oversized = yunlink::encode_payload(input);
    if (yunlink::decode_payload(oversized, &output)) {
        std::cerr << "oversized component list accepted\n";
        return 3;
    }

    return 0;
}
