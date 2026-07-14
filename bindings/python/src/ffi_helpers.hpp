#pragma once

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string>

#include <nanobind/nanobind.h>

#include "yunlink/c/yunlink_c.h"

namespace yunlink_python_ffi {

inline void throw_if_error(yunlink_result_t result) {
    if (result == YUNLINK_RESULT_OK) {
        return;
    }
    throw std::runtime_error(yunlink_result_name(result));
}

inline void copy_string(char* dst, size_t cap, const std::string& value) {
    std::memset(dst, 0, cap);
    const size_t n = std::min(cap - 1, value.size());
    std::memcpy(dst, value.data(), n);
}

inline yunlink_peer_t peer_from_python(const std::string& peer_id) {
    yunlink_peer_t peer{};
    copy_string(peer.id, sizeof(peer.id), peer_id);
    return peer;
}

inline yunlink_target_selector_t target_from_python(const nanobind::dict& target) {
    yunlink_target_selector_t out{};
    out.struct_size = sizeof(out);
    out.scope = nanobind::cast<uint8_t>(target["scope"]);
    out.target_type = nanobind::cast<uint8_t>(target["target_type"]);
    out.entity_id = nanobind::cast<uint32_t>(target["entity_id"]);
    out.group_id = nanobind::cast<uint32_t>(target["group_id"]);
    return out;
}

inline yunlink_vehicle_core_state_t state_from_python(const nanobind::dict& payload) {
    yunlink_vehicle_core_state_t out{};
    out.armed = nanobind::cast<bool>(payload["armed"]) ? 1 : 0;
    out.nav_mode = nanobind::cast<uint8_t>(payload["nav_mode"]);
    out.x_m = nanobind::cast<float>(payload["x_m"]);
    out.y_m = nanobind::cast<float>(payload["y_m"]);
    out.z_m = nanobind::cast<float>(payload["z_m"]);
    out.vx_mps = nanobind::cast<float>(payload["vx_mps"]);
    out.vy_mps = nanobind::cast<float>(payload["vy_mps"]);
    out.vz_mps = nanobind::cast<float>(payload["vz_mps"]);
    out.battery_percent = nanobind::cast<float>(payload["battery_percent"]);
    return out;
}

}  // namespace yunlink_python_ffi
