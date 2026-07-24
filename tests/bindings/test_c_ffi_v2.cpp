#include <cassert>
#include <cstring>

#include "yunlink/c/yunlink_v2.h"

namespace {

yunlink_v2_string_view_t text(const char* value) {
    return {value, std::strlen(value)};
}

}  // namespace

int main() {
    assert(yunlink_v2_abi_version() == 2);
    auto* runtime = yunlink_v2_runtime_create();
    assert(runtime != nullptr);
    const yunlink_v2_profile_view_t profiles[] = {
        {text("org.yunlink.mobility"), 1, 0, text("digest")},
    };
    yunlink_v2_runtime_config_t config{};
    config.struct_size = sizeof(config);
    config.endpoint_uid = text("ffi.endpoint");
    config.display_name = text("FFI endpoint");
    config.shared_secret = text("secret");
    config.tcp_listen_port = 19701;
    config.profiles = profiles;
    config.profile_count = 1;
    assert(yunlink_v2_runtime_start(runtime, &config) == 0);
    yunlink_v2_runtime_stop(runtime);
    yunlink_v2_runtime_destroy(runtime);
    return 0;
}
