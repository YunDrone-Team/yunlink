#include "ui/main_window.hpp"

#include <cctype>
#include <chrono>

#include "common/monitor_ui_style.hpp"
#include "common/sunray_status_format.hpp"

namespace {

constexpr uint64_t kCommandExecutionStatusStaleMs = 10000;
constexpr uint64_t kPx4StateStaleMs = 10000;
constexpr double kPi = 3.14159265358979323846;

bool is_blank_topic_value(const std::string& value) {
    if (value.empty()) {
        return true;
    }
    for (unsigned char ch : value) {
        if (!std::isspace(ch)) {
            return false;
        }
    }
    return true;
}

std::string normalize_topic_value(const std::string& value) {
    return is_blank_topic_value(value) ? std::string("<empty>") : value;
}

std::string topic_value_or_default(const MonitorTopicState& topic, const std::string& key) {
    const auto it = topic.latest.values.find(key);
    if (it != topic.latest.values.end()) {
        return normalize_topic_value(it->second);
    }
    return monitor_has_snapshot(topic.latest) ? std::string("--") : std::string("WAIT");
}

uint64_t wall_time_ms() {
    const auto now = std::chrono::system_clock::now();
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());
}

QString battery_label_style(double battery, bool has_value, bool stale) {
    if (stale || !has_value) {
        return "color:#6f6f6f;font-weight:600;";
    }
    if (battery < 0.20) {
        return "color:#da1e28;font-weight:700;";
    }
    if (battery < 0.35) {
        return "color:#b28600;font-weight:700;";
    }
    return "color:#198038;font-weight:700;";
}

}  // namespace

bool MainWindow::confirm_command_send(const QString& command, const QString& payload) const {
    if (backend_ == nullptr) {
        return false;
    }
    const auto snapshot = backend_->snapshot_connection();
    return monitor_ui::confirm_risky_command(
        const_cast<MainWindow*>(this), command, monitor_ui::command_context_text(snapshot), payload);
}

void MainWindow::stage_takeoff() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::TakeoffCommand cmd{};
    const QString payload = "仅发送起飞动作，载具侧使用当前控制配置";
    if (!confirm_command_send("TAKEOFF", payload)) {
        return;
    }
    backend_->send_takeoff(cmd);
}

void MainWindow::stage_land() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::LandCommand cmd{};
    const QString payload = "仅发送降落动作，载具侧使用当前控制配置";
    if (!confirm_command_send("LAND", payload)) {
        return;
    }
    backend_->send_land(cmd);
}

void MainWindow::stage_return() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::ReturnCommand cmd{};
    const QString payload = "仅发送返航动作，载具侧使用当前控制配置";
    if (!confirm_command_send("RETURN", payload)) {
        return;
    }
    backend_->send_return(cmd);
}

void MainWindow::stage_move_point() {
    if (backend_ == nullptr) {
        return;
    }
    yunlink::GotoCommand cmd;
    cmd.x_m = static_cast<float>(point_x_spin_->value());
    cmd.y_m = static_cast<float>(point_y_spin_->value());
    cmd.z_m = static_cast<float>(point_z_spin_->value());
    cmd.yaw_rad = static_cast<float>(point_yaw_spin_->value() * kPi / 180.0);
    backend_->send_goto(cmd);
}

void MainWindow::stage_move_velocity() {
    if (backend_ == nullptr) {
        return;
    }

    yunlink::VelocitySetpointCommand cmd;
    cmd.body_frame = false;
    cmd.vx_mps = static_cast<float>(vel_x_spin_->value());
    cmd.vy_mps = static_cast<float>(vel_y_spin_->value());
    cmd.vz_mps = static_cast<float>(vel_z_spin_->value());
    cmd.yaw_rate_radps = static_cast<float>(vel_yaw_rate_spin_->value() * kPi / 180.0);
    backend_->send_velocity_setpoint(cmd);
}

void MainWindow::refresh_command_controls() {
    if (backend_ == nullptr) {
        return;
    }

    const auto snapshot = backend_->snapshot_connection();
    const auto topics = backend_->snapshot_topics();
    const auto history = backend_->snapshot_command_history();
    const uint64_t now_ms = wall_time_ms();
    const bool session_ready =
        snapshot.runtime_started && snapshot.peer_ready && snapshot.session_id != 0;
    const bool authority_ready = backend_->can_send_commands();
    bool has_exec = false;
    bool exec_stale = false;
    bool ready_for_takeoff = false;
    bool ready_for_land = false;
    std::string command_name = "-";
    std::string execution_name = "-";
    std::string battery_text = "WAIT";
    double battery_value = 0.0;
    bool battery_has_value = false;
    bool battery_stale = false;
    std::string reason_text = "-";
    std::string ready_takeoff_text = "no";
    std::string ready_land_text = "no";

    const auto px4_it = topics.find("px4_state");
    if (px4_it != topics.end() && monitor_has_snapshot(px4_it->second.latest)) {
        const auto& topic = px4_it->second;
        const uint64_t age_ms =
            topic.latest.received_at_ms == 0 || now_ms < topic.latest.received_at_ms
                ? 0
                : now_ms - topic.latest.received_at_ms;
        if (topic.latest.received_at_ms == 0 || age_ms > kPx4StateStaleMs) {
            battery_text = "STALE";
            battery_stale = true;
        } else {
            const auto battery_it = topic.latest.values.find("battery_percentage");
            if (battery_it != topic.latest.values.end() &&
                monitor_parse_double(battery_it->second, &battery_value)) {
                battery_text = monitor_fmt_percent(battery_value);
                battery_has_value = true;
            } else {
                battery_text = "--";
            }
        }
    }

    const auto exec_it = topics.find("command_execution_status");
    if (exec_it != topics.end() && monitor_has_snapshot(exec_it->second.latest)) {
        const auto& topic = exec_it->second;
        const auto& values = topic.latest.values;
        const uint64_t age_ms =
            topic.latest.received_at_ms == 0 || now_ms < topic.latest.received_at_ms
                ? 0
                : now_ms - topic.latest.received_at_ms;
        exec_stale =
            topic.latest.received_at_ms == 0 || age_ms > kCommandExecutionStatusStaleMs;
        if (!exec_stale) {
            has_exec = true;
            const auto kind_it = values.find("command_kind_name");
            if (kind_it != values.end() && !kind_it->second.empty()) {
                command_name = kind_it->second;
            }
            const auto state_it = values.find("execution_state_name");
            if (state_it != values.end() && !state_it->second.empty()) {
                execution_name = state_it->second;
            }
            const auto reason_it = values.find("busy_reason");
            if (reason_it != values.end() && !is_blank_topic_value(reason_it->second)) {
                reason_text = reason_it->second;
            } else {
                const auto detail_it = values.find("detail");
                if (detail_it != values.end() && !is_blank_topic_value(detail_it->second)) {
                    reason_text = detail_it->second;
                }
            }
            monitor_parse_bool(topic_value_or_default(topic, "ready_for_takeoff"),
                               &ready_for_takeoff);
            monitor_parse_bool(topic_value_or_default(topic, "ready_for_land"),
                               &ready_for_land);
            ready_takeoff_text = ready_for_takeoff ? "yes" : "no";
            ready_land_text = ready_for_land ? "yes" : "no";
        } else {
            const auto kind_it = values.find("command_kind_name");
            if (kind_it != values.end() && !kind_it->second.empty()) {
                command_name = kind_it->second;
            }
            execution_name = "STALE";
            reason_text = "command_execution_status 快照已过期；age=" +
                          monitor_fmt_age_ms(age_ms) + "；已忽略旧的 ready/busy 门禁";
            ready_takeoff_text = "stale";
            ready_land_text = "stale";
        }
    }

    if (!history.empty()) {
        const auto& last = history.back();
        if (command_lifecycle_is_terminal(last.lifecycle)) {
            command_name = last.action;
            execution_name = command_lifecycle_label(last.lifecycle);
            if (!last.result_detail.empty()) {
                reason_text = last.result_detail;
            } else if (!last.execution_detail.empty()) {
                reason_text = last.execution_detail;
            } else {
                reason_text = command_lifecycle_label(last.lifecycle);
            }
            has_exec = false;
        }
    }

    if (current_command_value_ != nullptr) {
        current_command_value_->setText(QString::fromStdString(command_name));
    }
    if (current_execution_state_value_ != nullptr) {
        current_execution_state_value_->setText(QString::fromStdString(execution_name));
    }
    if (current_battery_value_ != nullptr) {
        current_battery_value_->setText(QString::fromStdString(battery_text));
        current_battery_value_->setStyleSheet(
            battery_label_style(battery_value, battery_has_value, battery_stale));
    }
    if (current_execution_reason_value_ != nullptr) {
        current_execution_reason_value_->setText(QString::fromStdString(reason_text));
    }
    if (current_ready_takeoff_value_ != nullptr) {
        current_ready_takeoff_value_->setText(QString::fromStdString(ready_takeoff_text));
    }
    if (current_ready_land_value_ != nullptr) {
        current_ready_land_value_->setText(QString::fromStdString(ready_land_text));
    }

    if (takeoff_button_ != nullptr) {
        takeoff_button_->setEnabled(session_ready && authority_ready && (!has_exec || ready_for_takeoff));
        land_button_->setEnabled(session_ready && authority_ready && (!has_exec || ready_for_land));
        return_button_->setEnabled(session_ready && authority_ready);
        point_button_->setEnabled(session_ready);
        velocity_button_->setEnabled(session_ready);
    }
    command_hint_label_->setText(monitor_ui::command_gate_notice(session_ready,
                                                                 authority_ready,
                                                                 exec_stale,
                                                                 has_exec,
                                                                 ready_for_takeoff,
                                                                 ready_for_land,
                                                                 command_name,
                                                                 execution_name,
                                                                 reason_text));
}
