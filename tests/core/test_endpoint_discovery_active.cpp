#include <iostream>

#include "yunlink/discovery/endpoint_discovery.hpp"

int main() {
    const std::string secret = "discovery-test-secret";
    yunlink::EndpointDiscoveryQuery query{0x123456789abcdef0ULL, 800U};
    const auto encoded_query = yunlink::encode_endpoint_discovery_query(query, secret);
    yunlink::EndpointDiscoveryQuery decoded_query{};
    std::string error;
    if (!yunlink::decode_endpoint_discovery_query(encoded_query, secret, &decoded_query, &error) ||
        decoded_query.nonce != query.nonce || decoded_query.response_window_ms != query.response_window_ms) {
        std::cerr << "主动发现 query round-trip failed: " << error << '\n';
        return 1;
    }
    if (yunlink::decode_endpoint_discovery_query(encoded_query, "wrong-secret", &decoded_query, &error)) {
        std::cerr << "主动发现 query accepted an invalid authentication tag\n";
        return 2;
    }

    yunlink::EndpointAdvertisement advertisement{};
    advertisement.endpoint_id = "a1b2c";
    advertisement.display_name_prefix = "uav";
    advertisement.agent_id = 7;
    advertisement.node_name = "bridge";
    advertisement.tcp_listen_port = 9696;
    advertisement.udp_bind_port = 9898;
    advertisement.capabilities = {"state", "commands", "config-resource-v1"};
    advertisement.sequence = 42;
    advertisement.discovery_period_ms = 15000;

    const auto reply = yunlink::encode_endpoint_discovery_reply(query.nonce, advertisement, secret);
    if (reply.size() > 128U) {
        std::cerr << "主动发现 reply exceeded compact packet budget\n";
        return 3;
    }
    uint64_t reply_nonce = 0;
    yunlink::EndpointAdvertisement decoded_reply{};
    if (!yunlink::decode_endpoint_discovery_reply(reply, secret, &reply_nonce, &decoded_reply, &error) ||
        reply_nonce != query.nonce || decoded_reply.endpoint_id != advertisement.endpoint_id ||
        decoded_reply.tcp_listen_port != advertisement.tcp_listen_port ||
        decoded_reply.discovery_period_ms != advertisement.discovery_period_ms ||
        decoded_reply.capabilities.size() != advertisement.capabilities.size()) {
        std::cerr << "主动发现 reply round-trip failed: " << error << '\n';
        return 4;
    }
    if (yunlink::decode_endpoint_discovery_reply(reply, "wrong-secret", &reply_nonce, &decoded_reply, &error)) {
        std::cerr << "主动发现 reply accepted an invalid authentication tag\n";
        return 5;
    }
    const yunlink::ByteBuffer truncated_reply(reply.begin(), reply.end() - 1U);
    if (yunlink::decode_endpoint_discovery_reply(truncated_reply, secret, &reply_nonce, &decoded_reply, &error)) {
        std::cerr << "主动发现 reply accepted a truncated payload\n";
        return 6;
    }

    const auto v1 = yunlink::encode_endpoint_advertisement(advertisement);
    yunlink::EndpointAdvertisement decoded_v1{};
    if (!yunlink::decode_endpoint_advertisement(v1, &decoded_v1, &error) ||
        decoded_v1.endpoint_id != advertisement.endpoint_id) {
        std::cerr << "V1 compatibility decode failed: " << error << '\n';
        return 7;
    }
    return 0;
}
