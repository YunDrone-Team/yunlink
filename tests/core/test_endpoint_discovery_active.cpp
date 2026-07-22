#include <chrono>
#include <iostream>
#include <thread>

#include "yunlink/discovery/endpoint_discovery.hpp"
#include "../bindings/test_socket_utils.hpp"

int main() {
    const std::string secret = "discovery-test-secret";
    if (yunlink::make_endpoint_display_name("SURY-uav", 1U, "a1b2c") != "SURY-uav1-a1b2c" ||
        yunlink::make_endpoint_display_name("SURY#-uav", 1U, "a1b2c") != "SURY#-uav1-a1b2c") {
        std::cerr << "SURY endpoint display-name convention failed\n";
        return 16;
    }
    if (!yunlink::validate_endpoint_id("a1b2c") || !yunlink::validate_endpoint_id("a1b2c3d4e5f6") ||
        yunlink::validate_endpoint_id("Alpha") || yunlink::validate_endpoint_id("ABCDEFGH") ||
        yunlink::validate_endpoint_id("A1") || yunlink::validate_endpoint_id("abcdefghi") ||
        yunlink::validate_endpoint_id("Alpha-") || yunlink::validate_endpoint_id("")) {
        std::cerr << "endpoint UID validation did not preserve generated IDs only\n";
        return 17;
    }
    yunlink::EndpointDiscoveryQuery query{0x123456789abcdef0ULL, 800U};
    const auto encoded_query = yunlink::encode_endpoint_discovery_query(query, secret);
    yunlink::EndpointDiscoveryQuery decoded_query{};
    std::string error;
    if (!yunlink::decode_endpoint_discovery_query(encoded_query, secret, &decoded_query, &error) ||
        decoded_query.nonce != query.nonce ||
        decoded_query.response_window_ms != query.response_window_ms) {
        std::cerr << "主动发现 query round-trip failed: " << error << '\n';
        return 1;
    }
    if (yunlink::decode_endpoint_discovery_query(
            encoded_query, "wrong-secret", &decoded_query, &error)) {
        std::cerr << "主动发现 query accepted an invalid authentication tag\n";
        return 2;
    }

    yunlink::EndpointAdvertisement advertisement{};
    advertisement.endpoint_id = "a1b2c";
    advertisement.display_name_prefix = "uav";
    advertisement.agent_type = "ugv";
    advertisement.agent_id = 7;
    advertisement.role = "relay";
    advertisement.node_name = "bridge";
    advertisement.tcp_listen_port = 9696;
    advertisement.udp_bind_port = 9898;
    advertisement.capabilities = {"state", "commands", "config-resource-v1", "topic-stream-v1"};
    advertisement.sequence = 42;
    advertisement.discovery_period_ms = 15000;

    const auto reply = yunlink::encode_endpoint_discovery_reply(query.nonce, advertisement, secret);
    if (reply.size() > 128U) {
        std::cerr << "主动发现 reply exceeded compact packet budget\n";
        return 3;
    }
    uint64_t reply_nonce = 0;
    yunlink::EndpointAdvertisement decoded_reply{};
    if (!yunlink::decode_endpoint_discovery_reply(
            reply, secret, &reply_nonce, &decoded_reply, &error) ||
        reply_nonce != query.nonce || decoded_reply.endpoint_id != advertisement.endpoint_id ||
        decoded_reply.display_name != "uav7-a1b2c" ||
        decoded_reply.agent_type != advertisement.agent_type ||
        decoded_reply.role != advertisement.role ||
        decoded_reply.tcp_listen_port != advertisement.tcp_listen_port ||
        decoded_reply.discovery_period_ms != advertisement.discovery_period_ms ||
        decoded_reply.capabilities.size() != advertisement.capabilities.size()) {
        std::cerr << "主动发现 reply round-trip failed: " << error << '\n';
        return 4;
    }
    if (yunlink::decode_endpoint_discovery_reply(
            reply, "wrong-secret", &reply_nonce, &decoded_reply, &error)) {
        std::cerr << "主动发现 reply accepted an invalid authentication tag\n";
        return 5;
    }
    const yunlink::ByteBuffer truncated_reply(reply.begin(), reply.end() - 1U);
    if (yunlink::decode_endpoint_discovery_reply(
            truncated_reply, secret, &reply_nonce, &decoded_reply, &error)) {
        std::cerr << "主动发现 reply accepted a truncated payload\n";
        return 6;
    }

    auto invalid_advertisement = advertisement;
    invalid_advertisement.agent_type = std::string(16U, 'u');
    if (!yunlink::encode_endpoint_discovery_reply(query.nonce, invalid_advertisement, secret)
             .empty()) {
        std::cerr << "active discovery accepted an oversized agent type\n";
        return 13;
    }
    invalid_advertisement = advertisement;
    invalid_advertisement.role = std::string(16U, 'r');
    if (!yunlink::encode_endpoint_discovery_reply(query.nonce, invalid_advertisement, secret)
             .empty()) {
        std::cerr << "active discovery accepted an oversized endpoint role\n";
        return 15;
    }
    invalid_advertisement = advertisement;
    invalid_advertisement.node_name = std::string(63U, 'n');
    invalid_advertisement.display_name_prefix = std::string(32U, 'p');
    if (!yunlink::encode_endpoint_discovery_reply(query.nonce, invalid_advertisement, secret)
             .empty()) {
        std::cerr << "active discovery emitted a reply beyond the packet budget\n";
        return 14;
    }

    const auto v1 = yunlink::encode_endpoint_advertisement(advertisement);
    yunlink::EndpointAdvertisement decoded_v1{};
    if (!yunlink::decode_endpoint_advertisement(v1, &decoded_v1, &error) ||
        decoded_v1.endpoint_id != advertisement.endpoint_id ||
        decoded_v1.display_name != "uav7-a1b2c") {
        std::cerr << "V1 compatibility decode failed: " << error << '\n';
        return 7;
    }

    const uint16_t discovery_port =
        yunlink::test_socket::find_free_port(yunlink::test_socket::SocketProtocol::kUdp);
    if (discovery_port == 0) {
        std::cerr << "failed to reserve same-host discovery test port\n";
        return 8;
    }

    yunlink::EndpointDiscoveryConfig same_host_config{};
    same_host_config.discovery_port = discovery_port;
    same_host_config.target_ip = "127.0.0.1";
    same_host_config.shared_secret = secret;
    same_host_config.io_poll_interval_ms = 1;

    yunlink::EndpointAdvertiser advertiser;
    if (advertiser.start(same_host_config) != yunlink::ErrorCode::kOk) {
        std::cerr << "same-host advertiser start failed: " << advertiser.last_error() << '\n';
        return 9;
    }
    advertiser.set_advertisement(advertisement);

    yunlink::EndpointListener listener;
    if (listener.start(same_host_config) != yunlink::ErrorCode::kOk) {
        std::cerr << "same-host listener start failed: " << listener.last_error() << '\n';
        return 10;
    }
    if (listener.send_query(query.nonce, query.response_window_ms) != yunlink::ErrorCode::kOk) {
        std::cerr << "same-host query failed: " << listener.last_error() << '\n';
        return 11;
    }

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    bool found_same_host_reply = false;
    while (std::chrono::steady_clock::now() < deadline && !found_same_host_reply) {
        std::vector<yunlink::EndpointAdvertisementPacket> packets;
        listener.drain(&packets);
        for (const auto& packet : packets) {
            if (packet.is_query_reply && packet.reply_nonce == query.nonce &&
                packet.advertisement.endpoint_id == advertisement.endpoint_id) {
                found_same_host_reply = true;
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    listener.stop();
    advertiser.stop();
    if (!found_same_host_reply) {
        std::cerr << "same-host active discovery reply was not delivered to the query socket\n";
        return 12;
    }
    return 0;
}
