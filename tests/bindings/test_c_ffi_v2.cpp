#include <cassert>
#include <chrono>
#include <cstring>
#include <thread>

#include "yunlink/c/yunlink_v2.h"

namespace {

yunlink_v2_string_view_t text(const char* value) {
    return {value, std::strlen(value)};
}

}  // namespace

int main() {
    assert(yunlink_v2_abi_version() == 2);
    auto* server = yunlink_v2_runtime_create();
    auto* client = yunlink_v2_runtime_create();
    assert(server != nullptr);
    assert(client != nullptr);
    const yunlink_v2_profile_view_t profiles[] = {
        {text("org.yunlink.mobility"), 1, 0, text("digest")},
    };
    yunlink_v2_runtime_config_t server_config{};
    server_config.struct_size = sizeof(server_config);
    server_config.endpoint_uid = text("ffi.server");
    server_config.display_name = text("FFI server");
    server_config.shared_secret = text("secret");
    server_config.tcp_listen_port = 19701;
    server_config.profiles = profiles;
    server_config.profile_count = 1;
    assert(yunlink_v2_runtime_start(server, &server_config) == 0);

    auto client_config = server_config;
    client_config.endpoint_uid = text("ffi.client");
    client_config.display_name = text("FFI client");
    client_config.tcp_listen_port = 19702;
    assert(yunlink_v2_runtime_start(client, &client_config) == 0);

    char endpoint_uid[129]{};
    assert(yunlink_v2_runtime_session_endpoint_uid(client, text("missing"), 1, endpoint_uid,
                                                   sizeof(endpoint_uid)) != 0);
    yunlink_v2_peer_t peer{};
    assert(yunlink_v2_runtime_connect(client, text("127.0.0.1"), 19701, &peer) == 0);
    const auto session_id = yunlink_v2_runtime_open_session(client, text(peer.id));
    assert(session_id != 0);
    uint16_t endpoint_result = 1;
    for (int attempt = 0; attempt < 100 && endpoint_result != 0; ++attempt) {
        endpoint_result = yunlink_v2_runtime_session_endpoint_uid(
            client, text(peer.id), session_id, endpoint_uid, sizeof(endpoint_uid));
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    assert(endpoint_result == 0);
    assert(std::strcmp(endpoint_uid, "ffi.server") == 0);
    char short_uid[2]{};
    assert(yunlink_v2_runtime_session_endpoint_uid(client, text(peer.id), session_id, short_uid,
                                                   sizeof(short_uid)) != 0);

    yunlink_v2_runtime_stop(client);
    yunlink_v2_runtime_stop(server);
    yunlink_v2_runtime_destroy(client);
    yunlink_v2_runtime_destroy(server);
    return 0;
}
