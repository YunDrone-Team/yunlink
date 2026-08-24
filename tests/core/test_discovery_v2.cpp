#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#include <asio.hpp>

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
    advertisement.attributes = {{"site", "lab-a"}};
    advertisement.entities = {{"entity:uav:1",
                               "uav",
                               "UAV 1",
                               Availability::kOnline,
                               1,
                               {{"hardware_id", "SIM-UAV-001"}, {"model", "sim"}}},
                              {"entity:ugv:27",
                               "ugv",
                               "UGV 27",
                               Availability::kDegraded,
                               27,
                               {{"hardware_id", "SIM-UGV-027"}}}};
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
    assert(decoded_reply.attributes.empty());
    assert(decoded_reply.entities.front().attributes.empty());
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
    assert(decoded_reply.attributes.empty());
    assert(decoded_reply.entities.front().attributes.empty());

    DiscoveryQuery v4_query{44, 250, kDiscoveryFormatV4};
    const Bytes encoded_v4_query = encode_discovery_query(v4_query, secret);
    assert(encoded_v4_query.size() == 22);
    assert(encoded_v4_query[3] == '4');
    assert(decode_discovery_query(encoded_v4_query, secret, &decoded_query));
    assert(decoded_query.format_version == kDiscoveryFormatV4);
    const Bytes v4_reply = encode_discovery_reply(v4_query, advertisement, secret);
    assert(!v4_reply.empty());
    assert(v4_reply[3] == '4');
    assert(decode_discovery_reply(v4_reply, secret, v4_query.nonce, &decoded_reply));
    assert(decoded_reply.attributes == advertisement.attributes);
    assert(decoded_reply.entities.front().attributes == advertisement.entities.front().attributes);
    assert(decoded_reply.entities.back().attributes == advertisement.entities.back().attributes);

    DiscoveryAdvertisement invalid_attributes = advertisement;
    invalid_attributes.attributes[""] = "invalid";
    assert(encode_discovery_reply(v4_query, invalid_attributes, secret).empty());

    Bytes v1 = encoded_reply;
    v1[3] = '1';
    assert(!decode_discovery_reply(v1, secret, v2_query.nonce, &decoded_reply));

    Bytes corrupted = encoded_reply;
    corrupted[10] ^= 0x80;
    assert(!decode_discovery_reply(corrupted, secret, v2_query.nonce, &decoded_reply));

    Bytes truncated_v3 = v3_reply;
    truncated_v3.pop_back();
    assert(!decode_discovery_reply(truncated_v3, secret, v3_query.nonce, &decoded_reply));

    Bytes truncated_v4 = v4_reply;
    truncated_v4.pop_back();
    assert(!decode_discovery_reply(truncated_v4, secret, v4_query.nonce, &decoded_reply));

    asio::io_context io;
    asio::ip::udp::socket reservation(io, asio::ip::udp::endpoint(asio::ip::udp::v4(), 0));
    const uint16_t advertiser_port = reservation.local_endpoint().port();
    reservation.close();

    std::mutex event_mutex;
    std::vector<DiscoveryAdvertiserEvent> events;
    DiscoveryAdvertiser advertiser;
    assert(advertiser.start(
               advertiser_port, advertisement, secret, [&](const DiscoveryAdvertiserEvent& event) {
                   std::lock_guard<std::mutex> lock(event_mutex);
                   events.push_back(event);
               }) == ErrorCode::kOk);

    asio::ip::udp::socket client(io, asio::ip::udp::endpoint(asio::ip::address_v4::loopback(), 0));
    const asio::ip::udp::endpoint destination(asio::ip::address_v4::loopback(), advertiser_port);
    client.send_to(asio::buffer(encoded_v3_query), destination);
    std::array<uint8_t, 4096> response{};
    asio::ip::udp::endpoint response_source;
    const size_t response_size = client.receive_from(asio::buffer(response), response_source);
    assert(response_size > 0);
    client.send_to(asio::buffer(Bytes{1, 2, 3}), destination);

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    while (std::chrono::steady_clock::now() < deadline) {
        {
            std::lock_guard<std::mutex> lock(event_mutex);
            if (events.size() >= 3)
                break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    advertiser.stop();

    const auto has_event = [&](DiscoveryAdvertiserEventKind kind) {
        return std::any_of(events.begin(), events.end(), [&](const auto& event) {
            return event.kind == kind && event.remote_ip == "127.0.0.1" && event.remote_port != 0;
        });
    };
    assert(has_event(DiscoveryAdvertiserEventKind::kQueryReceived));
    assert(has_event(DiscoveryAdvertiserEventKind::kReplySent));
    assert(has_event(DiscoveryAdvertiserEventKind::kRejected));

    std::cout << "test_discovery_v2 passed\n";
    return 0;
}
