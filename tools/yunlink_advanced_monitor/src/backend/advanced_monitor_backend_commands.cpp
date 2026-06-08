#include "backend/advanced_monitor_backend.hpp"

void AdvancedMonitorBackend::send_takeoff(const yunlink::TakeoffCommand& cmd) {
    const std::string action = monitor_cmd_name_takeoff();
    const std::string detail = make_takeoff_draft(cmd).detail;
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, "命令未发送，session 未就绪: " + action);
        return;
    }
    request_command_authority_if_needed();
    yunlink::CommandHandle handle{};
    const auto ec = runtime_.command_publisher().publish_takeoff(
        peer_id, session_id, command_target(), cmd, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kCommand,
            "命令发送失败: " + action + " | ec=" + error_code_label(ec) + " | detail=" + detail);
        return;
    }
    record_command_sent(action, detail, handle);
    log_command_handle(action, handle, detail);
}

void AdvancedMonitorBackend::send_land(const yunlink::LandCommand& cmd) {
    const std::string action = monitor_cmd_name_land();
    const std::string detail = make_land_draft(cmd).detail;
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, "命令未发送，session 未就绪: " + action);
        return;
    }
    request_command_authority_if_needed();
    yunlink::CommandHandle handle{};
    const auto ec = runtime_.command_publisher().publish_land(
        peer_id, session_id, command_target(), cmd, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kCommand,
            "命令发送失败: " + action + " | ec=" + error_code_label(ec) + " | detail=" + detail);
        return;
    }
    record_command_sent(action, detail, handle);
    log_command_handle(action, handle, detail);
}

void AdvancedMonitorBackend::send_return(const yunlink::ReturnCommand& cmd) {
    const std::string action = monitor_cmd_name_return();
    const std::string detail = make_return_draft(cmd).detail;
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, "命令未发送，session 未就绪: " + action);
        return;
    }
    request_command_authority_if_needed();
    yunlink::CommandHandle handle{};
    const auto ec = runtime_.command_publisher().publish_return(
        peer_id, session_id, command_target(), cmd, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kCommand,
            "命令发送失败: " + action + " | ec=" + error_code_label(ec) + " | detail=" + detail);
        return;
    }
    record_command_sent(action, detail, handle);
    log_command_handle(action, handle, detail);
}

void AdvancedMonitorBackend::send_goto(const yunlink::GotoCommand& cmd) {
    const std::string action = monitor_cmd_name_goto();
    const std::string detail = make_goto_draft(cmd).detail;
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, "命令未发送，session 未就绪: " + action);
        return;
    }
    request_command_authority_if_needed();
    yunlink::CommandHandle handle{};
    const auto ec =
        runtime_.command_publisher().publish_goto(peer_id, session_id, command_target(), cmd, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kCommand,
            "命令发送失败: " + action + " | ec=" + error_code_label(ec) + " | detail=" + detail);
        return;
    }
    record_command_sent(action, detail, handle);
    log_command_handle(action, handle, detail);
}

void AdvancedMonitorBackend::send_velocity_setpoint(const yunlink::VelocitySetpointCommand& cmd) {
    const std::string action = monitor_cmd_name_velocity(cmd.body_frame);
    const std::string detail = make_velocity_draft(cmd).detail;
    std::string peer_id;
    uint64_t session_id = 0;
    if (!snapshot_send_context(&peer_id, &session_id)) {
        log(MonitorLogLevel::kWarn, MonitorLogSource::kCommand, "命令未发送，session 未就绪: " + action);
        return;
    }
    request_command_authority_if_needed();
    yunlink::CommandHandle handle{};
    const auto ec = runtime_.command_publisher().publish_velocity_setpoint(
        peer_id, session_id, command_target(), cmd, &handle);
    if (ec != yunlink::ErrorCode::kOk) {
        log(MonitorLogLevel::kError,
            MonitorLogSource::kCommand,
            "命令发送失败: " + action + " | ec=" + error_code_label(ec) + " | detail=" + detail);
        return;
    }
    record_command_sent(action, detail, handle);
    log_command_handle(action, handle, detail);
}
