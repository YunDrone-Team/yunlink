use eframe::egui;

use super::{MonitorApp, PendingHighRiskCommand};
use crate::model::{CommandLifecycle, MonitorState};
use crate::runtime_client::RuntimeCommand;

/// Render all editable command payload controls.
pub(super) fn command_inputs(app: &mut MonitorApp, ui: &mut egui::Ui) {
    let session_ready = app.state.peer_ready && app.state.session_id != 0;
    ui.horizontal_wrapped(|ui| {
        ui.label("高风险动作");
        if ui
            .add_enabled(session_ready, egui::Button::new("发送 TAKEOFF"))
            .clicked()
        {
            app.pending_high_risk = Some(PendingHighRiskCommand::Takeoff);
        }
        if ui
            .add_enabled(session_ready, egui::Button::new("发送 LAND"))
            .clicked()
        {
            app.pending_high_risk = Some(PendingHighRiskCommand::Land);
        }
        if ui
            .add_enabled(session_ready, egui::Button::new("发送 RETURN"))
            .clicked()
        {
            app.pending_high_risk = Some(PendingHighRiskCommand::Return);
        }
        ui.label("动作命令不携带飞行参数，使用载具侧当前控制配置");
    });
    ui.add_space(6.0);
    ui.columns(2, |columns| {
        columns[0].group(|ui| {
            ui.strong("目标点 (GOTO)");
            ui.horizontal_wrapped(|ui| {
                ui.add(
                    egui::DragValue::new(&mut app.goto_x_m)
                        .prefix("x ")
                        .suffix(" m"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.goto_y_m)
                        .prefix("y ")
                        .suffix(" m"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.goto_z_m)
                        .prefix("z ")
                        .suffix(" m"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.goto_yaw_rad)
                        .prefix("yaw ")
                        .suffix(" rad"),
                );
            });
            if ui
                .add_enabled(session_ready, egui::Button::new("发送 GOTO"))
                .clicked()
            {
                app.client.send(RuntimeCommand::Goto {
                    x_m: app.goto_x_m,
                    y_m: app.goto_y_m,
                    z_m: app.goto_z_m,
                    yaw_rad: app.goto_yaw_rad,
                });
            }
        });
        columns[1].group(|ui| {
            ui.strong("速度控制 (VELOCITY)");
            ui.horizontal_wrapped(|ui| {
                ui.add(
                    egui::DragValue::new(&mut app.velocity_vx_mps)
                        .prefix("vx ")
                        .suffix(" m/s"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.velocity_vy_mps)
                        .prefix("vy ")
                        .suffix(" m/s"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.velocity_vz_mps)
                        .prefix("vz ")
                        .suffix(" m/s"),
                );
                ui.add(
                    egui::DragValue::new(&mut app.velocity_yaw_rate_radps)
                        .prefix("yaw_rate ")
                        .suffix(" rad/s"),
                );
            });
            ui.checkbox(&mut app.velocity_body_frame, "机体系 (body frame)");
            if ui
                .add_enabled(session_ready, egui::Button::new("发送 VELOCITY"))
                .clicked()
            {
                app.client.send(RuntimeCommand::Velocity {
                    vx_mps: app.velocity_vx_mps,
                    vy_mps: app.velocity_vy_mps,
                    vz_mps: app.velocity_vz_mps,
                    yaw_rate_radps: app.velocity_yaw_rate_radps,
                    body_frame: app.velocity_body_frame,
                });
            }
        });
    });
}

/// Require a second explicit operator action before publishing a high-risk command.
pub(super) fn high_risk_confirmation(app: &mut MonitorApp, ctx: &egui::Context) {
    let Some(command) = app.pending_high_risk else {
        return;
    };
    let command_name = match command {
        PendingHighRiskCommand::Takeoff => "TAKEOFF",
        PendingHighRiskCommand::Land => "LAND",
        PendingHighRiskCommand::Return => "RETURN",
    };
    let consequence = match command {
        PendingHighRiskCommand::Takeoff => "载具将按当前控制配置执行起飞。",
        PendingHighRiskCommand::Land => "载具将立即进入降落流程。",
        PendingHighRiskCommand::Return => "载具将立即进入返航流程。",
    };
    let mut send = false;
    let mut cancel = false;
    egui::Window::new("确认高风险指令")
        .collapsible(false)
        .resizable(false)
        .anchor(egui::Align2::CENTER_CENTER, [0.0, 0.0])
        .show(ctx, |ui| {
            ui.strong(format!("指令：{command_name}"));
            ui.label("目标：UAV#1");
            ui.label(consequence);
            ui.label("发送后请在命令历史中核对 SENT、ACK 与最终执行结果。");
            ui.add_space(8.0);
            ui.horizontal(|ui| {
                if ui.button("取消").clicked() {
                    cancel = true;
                }
                if ui.button("确认发送").clicked() {
                    send = true;
                }
            });
        });

    if send {
        let runtime_command = match command {
            PendingHighRiskCommand::Takeoff => RuntimeCommand::Takeoff,
            PendingHighRiskCommand::Land => RuntimeCommand::Land,
            PendingHighRiskCommand::Return => RuntimeCommand::Return,
        };
        app.client.send(runtime_command);
        app.pending_high_risk = None;
    } else if cancel {
        app.pending_high_risk = None;
    }
}

/// Render command history with message/correlation information surfaced from C ABI.
pub(super) fn command_history(state: &MonitorState, ui: &mut egui::Ui) {
    egui::ScrollArea::vertical().show(ui, |ui| {
        egui::Grid::new("command-history-grid")
            .striped(true)
            .num_columns(7)
            .show(ui, |ui| {
                ui.strong("序号");
                ui.strong("发送时间");
                ui.strong("指令");
                ui.strong("状态");
                ui.strong("Session");
                ui.strong("Message ID");
                ui.strong("详情 / 回执");
                ui.end_row();
                for entry in &state.command_history {
                    ui.monospace(entry.sequence.to_string());
                    ui.monospace(entry.sent_at_ms.to_string());
                    ui.label(&entry.action);
                    lifecycle_label(ui, entry.lifecycle);
                    ui.monospace(entry.session_id.to_string());
                    ui.monospace(entry.message_id.to_string());
                    ui.label(if entry.result_detail.is_empty() {
                        &entry.detail
                    } else {
                        &entry.result_detail
                    });
                    ui.end_row();
                }
            });
    });
}

/// Render a lifecycle label with lightweight status coloring.
fn lifecycle_label(ui: &mut egui::Ui, lifecycle: CommandLifecycle) {
    let color = match lifecycle {
        CommandLifecycle::Succeeded => egui::Color32::from_rgb(36, 138, 61),
        CommandLifecycle::Failed | CommandLifecycle::Cancelled | CommandLifecycle::Expired => {
            egui::Color32::from_rgb(180, 56, 56)
        }
        CommandLifecycle::Sent | CommandLifecycle::Received => egui::Color32::from_rgb(64, 91, 160),
    };
    let label = match lifecycle {
        CommandLifecycle::Sent => "已发送 (SENT)",
        CommandLifecycle::Received => "已接收 (RECEIVED)",
        CommandLifecycle::Succeeded => "成功 (SUCCEEDED)",
        CommandLifecycle::Failed => "失败 (FAILED)",
        CommandLifecycle::Cancelled => "已取消 (CANCELLED)",
        CommandLifecycle::Expired => "已过期 (EXPIRED)",
    };
    ui.colored_label(color, label);
}
