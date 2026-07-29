#include <cassert>
#include <iostream>

#include "yunlink/discovery/discovery_v2.hpp"

int main() {
    using namespace yunlink::v2;

    const std::string secret = "test-secret";
    const DiscoveryQuery v2_query{42, 250, kDiscoveryFormatV2};
    const Bytes encoded_query = encode_discovery_query(v2_query, secret);
    const Bytes expected_query{0x59, 0x4c, 0x51, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x2a, 0x00, 0xfa, 0x3b, 0x90, 0xce, 0xfe, 0x7c, 0x41, 0x5a, 0x06};
    assert(encoded_query == expected_query);
    assert(encoded_query.size() == 22);
    DiscoveryQuery decoded_query;
    assert(decode_discovery_query(encoded_query, secret, &decoded_query));
    assert(decoded_query.nonce == v2_query.nonce);
    assert(decoded_query.format_version == kDiscoveryFormatV2);
    assert(!decode_discovery_query(encoded_query, "wrong", &decoded_query));
    assert(encode_discovery_query({42, 250, 1}, secret).empty());

    DiscoveryAdvertisement advertisement;
    advertisement.endpoint_uid = "endpoint:alpha";
    advertisement.display_name = "yunlink-endpoint";
    advertisement.tcp_listen_port = 9696;
    advertisement.capabilities = {"stream", "action"};
    advertisement.profiles = {{"org.yunlink.mobility", 1, 0, "mobility-v1"},
                              {"com.example.demo", 2, 3, "demo-v2.3"}};
    advertisement.entities = {{"entity:uav:1", "uav", "UAV 1", Availability::kOnline, 1},
                              {"entity:ugv:27", "ugv", "UGV 27", Availability::kDegraded, 27}};
    advertisement.started_at_ms = 1000;
    advertisement.sequence = 7;

    const Bytes encoded_reply = encode_discovery_reply(v2_query, advertisement, secret);
    assert(!encoded_reply.empty());
    DiscoveryAdvertisement decoded_reply;
    assert(decode_discovery_reply(encoded_reply, secret, v2_query.nonce, &decoded_reply));
    assert(decoded_reply.endpoint_uid == advertisement.endpoint_uid);
    assert(decoded_reply.profiles.size() == 2);
    assert(decoded_reply.entities.size() == 2);
    assert(decoded_reply.entities.front().kind == "uav");
    // Legacy discovery-v2 remains decodable but intentionally cannot carry Agent IDs.
    assert(decoded_reply.entities.front().agent_id == 0);
    assert(decoded_reply.entities.back().agent_id == 0);
    assert(!decode_discovery_reply(encoded_reply, secret, v2_query.nonce + 1, &decoded_reply));

    DiscoveryQuery v3_query{43, 250, kDiscoveryFormatV3};
    const Bytes encoded_v3_query = encode_discovery_query(v3_query, secret);
    assert(encoded_v3_query.size() == 22);
    assert(encoded_v3_query[3] == '3');
    assert(decode_discovery_query(encoded_v3_query, secret, &decoded_query));
    assert(decoded_query.format_version == kDiscoveryFormatV3);
    const Bytes v3_reply = encode_discovery_reply(v3_query, advertisement, secret);
    assert(!v3_reply.empty());
    assert(v3_reply[3] == '3');
    assert(decode_discovery_reply(v3_reply, secret, v3_query.nonce, &decoded_reply));
    assert(decoded_reply.entities.size() == 2);
    assert(decoded_reply.entities.front().agent_id == 1);
    assert(decoded_reply.entities.back().agent_id == 27);

    Bytes v1 = encoded_reply;
    v1[3] = '1';
    assert(!decode_discovery_reply(v1, secret, v2_query.nonce, &decoded_reply));

    Bytes corrupted = encoded_reply;
    corrupted[10] ^= 0x80;
    assert(!decode_discovery_reply(corrupted, secret, v2_query.nonce, &decoded_reply));

    Bytes truncated_v3 = v3_reply;
    truncated_v3.pop_back();
    assert(!decode_discovery_reply(truncated_v3, secret, v3_query.nonce, &decoded_reply));

    std::cout << "test_discovery_v2 passed\n";
    return 0;
}
