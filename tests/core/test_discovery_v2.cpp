#include <cassert>
#include <iostream>

#include "yunlink/discovery/discovery_v2.hpp"

int main() {
    using namespace yunlink::v2;

    const std::string secret = "test-secret";
    const DiscoveryQuery query{42, 250};
    const Bytes encoded_query = encode_discovery_query(query, secret);
    assert(encoded_query.size() == 22);
    DiscoveryQuery decoded_query;
    assert(decode_discovery_query(encoded_query, secret, &decoded_query));
    assert(decoded_query.nonce == query.nonce);
    assert(!decode_discovery_query(encoded_query, "wrong", &decoded_query));

    DiscoveryAdvertisement advertisement;
    advertisement.endpoint_uid = "endpoint:alpha";
    advertisement.display_name = "yunlink-endpoint";
    advertisement.tcp_listen_port = 9696;
    advertisement.capabilities = {"stream", "action"};
    advertisement.profiles = {{"org.yunlink.mobility", 1, 0, "mobility-v1"},
                              {"com.example.demo", 2, 3, "demo-v2.3"}};
    advertisement.entities = {{"entity:1", "mobile-platform", "Unit 1", Availability::kOnline}};
    advertisement.started_at_ms = 1000;
    advertisement.sequence = 7;

    const Bytes encoded_reply = encode_discovery_reply(query, advertisement, secret);
    assert(!encoded_reply.empty());
    DiscoveryAdvertisement decoded_reply;
    assert(decode_discovery_reply(encoded_reply, secret, query.nonce, &decoded_reply));
    assert(decoded_reply.endpoint_uid == advertisement.endpoint_uid);
    assert(decoded_reply.profiles.size() == 2);
    assert(decoded_reply.entities.size() == 1);
    assert(decoded_reply.entities.front().kind == "mobile-platform");
    assert(!decode_discovery_reply(encoded_reply, secret, query.nonce + 1, &decoded_reply));

    Bytes v1 = encoded_reply;
    v1[3] = '1';
    assert(!decode_discovery_reply(v1, secret, query.nonce, &decoded_reply));

    Bytes corrupted = encoded_reply;
    corrupted[10] ^= 0x80;
    assert(!decode_discovery_reply(corrupted, secret, query.nonce, &decoded_reply));

    std::cout << "test_discovery_v2 passed\n";
    return 0;
}
