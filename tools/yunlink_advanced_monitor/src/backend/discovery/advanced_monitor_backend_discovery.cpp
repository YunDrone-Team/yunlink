#include "backend/advanced_monitor_backend.hpp"

#include <algorithm>

namespace {

std::string join_capabilities(const std::vector<std::string>& capabilities) {
    std::string out;
    for (std::size_t index = 0; index < capabilities.size(); ++index) {
        if (index != 0U) {
            out += ",";
        }
        out += capabilities[index];
    }
    return out;
}

std::string device_status_text(const DiscoveryDevice& device) {
    return device.stale ? "STALE" : "ACTIVE";
}

}  // namespace

void AdvancedMonitorBackend::poll_discovery() {
    if (!discovery_listener_started_) {
        return;
    }

    std::vector<yunlink::EndpointAdvertisementPacket> packets;
    discovery_listener_.drain(&packets);
    if (!packets.empty()) {
        std::lock_guard<std::mutex> lock(mu_);
        for (const auto& packet : packets) {
            DiscoveryDevice device{};
            device.endpoint_id = packet.advertisement.endpoint_id;
            device.display_name_prefix = packet.advertisement.display_name_prefix;
            device.display_name = packet.advertisement.display_name;
            device.agent_type = packet.advertisement.agent_type;
            device.agent_id = packet.advertisement.agent_id;
            device.role = packet.advertisement.role;
            device.source_ip = packet.source_ip;
            device.source_port = packet.source_port;
            device.tcp_listen_port = packet.advertisement.tcp_listen_port;
            device.udp_bind_port = packet.advertisement.udp_bind_port;
            device.node_name = packet.advertisement.node_name;
            device.protocol_version = packet.advertisement.protocol_version;
            device.capabilities = packet.advertisement.capabilities;
            device.last_seen_ms = packet.received_at_ms;
            device.started_at_ms = packet.advertisement.started_at_ms;
            device.sequence = packet.advertisement.sequence;
            device.discovery_period_ms = packet.advertisement.discovery_period_ms;
            device.stale = false;
            device.selected = selected_discovery_key_ == make_discovery_key(packet.source_ip,
                                                                            packet.advertisement.endpoint_id,
                                                                            packet.advertisement.tcp_listen_port);
            device.dedupe_key =
                make_discovery_key(packet.source_ip,
                                   packet.advertisement.endpoint_id,
                                   packet.advertisement.tcp_listen_port);
            discovery_devices_[device.dedupe_key] = std::move(device);
        }
    }

    const uint64_t now_ms = wall_time_ms();
    {
        std::lock_guard<std::mutex> lock(mu_);
        for (auto& entry : discovery_devices_) {
            DiscoveryDevice& device = entry.second;
            const uint64_t timeout_ms =
                static_cast<uint64_t>(std::max<uint32_t>(1000U, device.discovery_period_ms * 3U));
            device.stale = now_ms > device.last_seen_ms + timeout_ms;
            device.selected = selected_discovery_key_ == device.dedupe_key;
        }
    }

    const std::string error = discovery_listener_.last_error();
    if (!error.empty()) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kConnection, error);
    }
}

std::vector<DiscoveryDevice> AdvancedMonitorBackend::snapshot_discovery_devices() const {
    std::vector<DiscoveryDevice> devices;
    std::lock_guard<std::mutex> lock(mu_);
    devices.reserve(discovery_devices_.size());
    for (const auto& entry : discovery_devices_) {
        devices.push_back(entry.second);
    }

    std::sort(devices.begin(), devices.end(), [](const DiscoveryDevice& left, const DiscoveryDevice& right) {
        if (left.stale != right.stale) {
            return !left.stale && right.stale;
        }
        if (left.last_seen_ms != right.last_seen_ms) {
            return left.last_seen_ms > right.last_seen_ms;
        }
        return left.display_name < right.display_name;
    });
    return devices;
}

bool AdvancedMonitorBackend::connect_to_discovered_device(const std::string& dedupe_key) {
    DiscoveryDevice device;
    std::string previous_peer_id;
    bool had_previous_peer = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        const auto it = discovery_devices_.find(dedupe_key);
        if (it == discovery_devices_.end()) {
            return false;
        }
        device = it->second;
        selected_discovery_key_ = dedupe_key;
        remote_ip_ = device.source_ip;
        remote_tcp_port_ = static_cast<int>(device.tcp_listen_port);
        agent_id_ = static_cast<int>(device.agent_id);
        connection_.remote_endpoint = device.source_ip + ":" + std::to_string(device.tcp_listen_port);
        connection_.udp_target_endpoint = device.source_ip + ":" + std::to_string(clamp_port(udp_target_port_));
        connection_.last_note = "切换到发现设备 " + device.display_name;
        connection_.updated_at_ms = wall_time_ms();
        if (peer_ready_ && !peer_id_.empty()) {
            previous_peer_id = peer_id_;
            had_previous_peer = true;
        }
        peer_ready_ = false;
        peer_id_.clear();
        session_id_ = 0;
        authority_pending_ = false;
        authority_request_at_ms_ = 0;
        authority_expires_at_ms_ = 0;
        connection_.peer_ready = false;
        connection_.peer_id.clear();
        connection_.session_id = 0;
        connection_.session_state = "DISCOVERY_SELECTED";
    }

    if (had_previous_peer) {
        runtime_.tcp_clients().close_peer(previous_peer_id);
    }

    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kConnection,
        "选择发现设备 " + device.display_name + " ip=" + device.source_ip + " tcp=" +
            std::to_string(device.tcp_listen_port) + " capabilities=" +
            join_capabilities(device.capabilities));
    poll_runtime();
    return true;
}

bool AdvancedMonitorBackend::disconnect_current_device() {
    std::string previous_peer_id;
    bool had_selected_target = false;
    bool had_previous_peer = false;
    {
        std::lock_guard<std::mutex> lock(mu_);
        had_selected_target = !selected_discovery_key_.empty() || (!remote_ip_.empty() && remote_tcp_port_ > 0);
        if (!had_selected_target && (!peer_ready_ || peer_id_.empty())) {
            return false;
        }
        if (peer_ready_ && !peer_id_.empty()) {
            previous_peer_id = peer_id_;
            had_previous_peer = true;
        }

        selected_discovery_key_.clear();
        remote_ip_.clear();
        remote_tcp_port_ = 0;
        peer_ready_ = false;
        peer_id_.clear();
        session_id_ = 0;
        authority_pending_ = false;
        authority_request_at_ms_ = 0;
        authority_expires_at_ms_ = 0;
        connection_.peer_ready = false;
        connection_.peer_id.clear();
        connection_.session_id = 0;
        connection_.remote_endpoint = "-";
        connection_.udp_target_endpoint = "-";
        connection_.session_state = "DISCONNECTED";
        connection_.last_note = "已断开当前设备，等待重新选择";
        connection_.last_error.clear();
        connection_.updated_at_ms = wall_time_ms();
    }

    if (had_previous_peer) {
        runtime_.tcp_clients().close_peer(previous_peer_id);
    }

    log(MonitorLogLevel::kInfo, MonitorLogSource::kConnection, "用户断开当前设备连接");
    poll_runtime();
    return true;
}

std::string AdvancedMonitorBackend::selected_discovery_device_key() const {
    std::lock_guard<std::mutex> lock(mu_);
    return selected_discovery_key_;
}

std::string AdvancedMonitorBackend::make_discovery_key(const std::string& source_ip,
                                                       const std::string& endpoint_id,
                                                       uint16_t tcp_listen_port) {
    return endpoint_id + "|" + source_ip + "|" + std::to_string(tcp_listen_port);
}

void AdvancedMonitorBackend::start_discovery_listener() {
    yunlink::EndpointDiscoveryConfig config;
    config.discovery_port = clamp_port(discovery_port_);
    config.target_ip = discovery_target_ip_;

    const auto ec = discovery_listener_.start(config);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kWarn,
            MonitorLogSource::kConnection,
            "Discovery listener 启动失败，port=" + std::to_string(config.discovery_port) +
                " ec=" + error_code_label(ec));
        return;
    }

    discovery_listener_started_ = true;
    log(MonitorLogLevel::kInfo,
        MonitorLogSource::kConnection,
        "Discovery listener 已启动，listen=0.0.0.0:" + std::to_string(config.discovery_port));
}

void AdvancedMonitorBackend::update_discovery_snapshot_unlocked(const DiscoveryDevice& device) {
    if (connection_.runtime_status.empty()) {
        return;
    }
    connection_.last_note = "当前发现设备: " + device.display_name + " [" + device_status_text(device) + "]";
}
