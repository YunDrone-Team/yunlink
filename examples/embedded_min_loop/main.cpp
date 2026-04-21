/**
 * @file examples/embedded_min_loop/main.cpp
 * @brief sunray_communication_lib source file.
 */

#include <chrono>
#include <iostream>
#include <thread>

#include "sunraycom/c/sunraycom_c.h"

int main() {
    sunraycom_handle_t* h = sunraycom_create();
    if (!h)
        return 1;

    sunraycom_runtime_config_t cfg{};
    cfg.udp_bind_port = 9696;
    cfg.udp_target_port = 9898;
    cfg.tcp_listen_port = 9696;
    cfg.connect_timeout_ms = 5000;
    cfg.io_poll_interval_ms = 5;
    cfg.max_buffer_bytes_per_peer = 1 << 20;

    if (sunraycom_start(h, &cfg) != SUNRAYCOM_STATUS_OK) {
        std::cerr << "start failed\n";
        sunraycom_destroy(h);
        return 2;
    }

    for (int i = 0; i < 100; ++i) {
        sunraycom_event_t ev{};
        sunraycom_poll_event(h, &ev);
        if (ev.type == SUNRAYCOM_EVENT_ENVELOPE) {
            std::cout << "envelope family=" << static_cast<int>(ev.message_family)
                      << " type=" << ev.message_type << " peer=" << ev.peer_id << "\n";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    sunraycom_stop(h);
    sunraycom_destroy(h);
    return 0;
}
