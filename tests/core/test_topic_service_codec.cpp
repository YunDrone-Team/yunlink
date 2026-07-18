/** @file @brief Topic directory, subscription and sample schema-1 codec contract. */

#include <iostream>
#include <string>

#include "yunlink/core/semantic_messages.hpp"

int main() {
    static_assert(yunlink::MessageTraits<yunlink::TopicListRequest>::kSchemaVersion == 1,
                  "topic service must remain in schema 1");
    static_assert(yunlink::MessageTraits<yunlink::TopicSample>::kSchemaVersion == 1,
                  "topic sample must remain in schema 1");

    yunlink::TopicListResponse list{};
    list.success = true;
    list.message = "ok";
    list.revision = "catalog-42";
    list.topics = {{"/uav7/sunray/px4_state", "sunray_msgs/Px4State", 1},
                   {"/camera/image/compressed", "sensor_msgs/CompressedImage", 2}};
    const auto list_bytes = yunlink::encode_payload(list);
    yunlink::TopicListResponse decoded_list{};
    if (list_bytes.empty() || !yunlink::decode_payload(list_bytes, &decoded_list) ||
        decoded_list.revision != list.revision || decoded_list.topics.size() != 2 ||
        decoded_list.topics[1].publisher_count != 2) {
        std::cerr << "topic list round-trip failed\n";
        return 1;
    }

    yunlink::TopicSubscriptionRequest request{};
    request.topic_name = "/camera/image/compressed";
    request.subscribe = true;
    request.max_rate_hz = 5.0F;
    request.max_payload_bytes = 262144;
    const auto request_bytes = yunlink::encode_payload(request);
    yunlink::TopicSubscriptionRequest decoded_request{};
    if (request_bytes.empty() || !yunlink::decode_payload(request_bytes, &decoded_request) ||
        decoded_request.topic_name != request.topic_name || !decoded_request.subscribe ||
        decoded_request.max_rate_hz != request.max_rate_hz ||
        decoded_request.max_payload_bytes != request.max_payload_bytes) {
        std::cerr << "topic subscription request round-trip failed\n";
        return 2;
    }

    yunlink::TopicSubscriptionResponse response{};
    response.success = true;
    response.message = "subscribed";
    response.topic_name = request.topic_name;
    response.subscribed = true;
    response.type_name = "sensor_msgs/CompressedImage";
    response.max_rate_hz = 5.0F;
    response.max_payload_bytes = 262144;
    const auto response_bytes = yunlink::encode_payload(response);
    yunlink::TopicSubscriptionResponse decoded_response{};
    if (response_bytes.empty() || !yunlink::decode_payload(response_bytes, &decoded_response) ||
        decoded_response.type_name != response.type_name || !decoded_response.subscribed) {
        std::cerr << "topic subscription response round-trip failed\n";
        return 3;
    }

    yunlink::TopicSample sample{};
    sample.topic_name = request.topic_name;
    sample.type_name = response.type_name;
    sample.type_hash = "8f7a12909da2c9d3332d540a0977563f";
    sample.encoding = "ros1";
    sample.message_definition = "std_msgs/Header header\nstring format\nuint8[] data\n";
    sample.receive_time_ns = 123456789ULL;
    sample.sequence = 9;
    sample.metadata_included = true;
    sample.data = {0, 1, 2, 3, 255};
    const auto sample_bytes = yunlink::encode_payload(sample);
    yunlink::TopicSample decoded_sample{};
    if (sample_bytes.empty() || !yunlink::decode_payload(sample_bytes, &decoded_sample) ||
        decoded_sample.topic_name != sample.topic_name ||
        decoded_sample.message_definition != sample.message_definition ||
        decoded_sample.sequence != sample.sequence || !decoded_sample.metadata_included ||
        decoded_sample.data != sample.data) {
        std::cerr << "topic sample round-trip failed\n";
        return 4;
    }

    auto truncated = sample_bytes;
    truncated.pop_back();
    if (yunlink::decode_payload(truncated, &decoded_sample)) {
        std::cerr << "truncated topic sample accepted\n";
        return 5;
    }

    sample.message_definition.assign(4U * 1024U * 1024U + 1U, 'x');
    if (!yunlink::encode_payload(sample).empty()) {
        std::cerr << "oversized topic metadata encoded\n";
        return 6;
    }
    sample.message_definition.clear();
    sample.data.assign(4U * 1024U * 1024U + 1U, 0);
    if (!yunlink::encode_payload(sample).empty()) {
        std::cerr << "oversized topic payload encoded\n";
        return 7;
    }
    return 0;
}
