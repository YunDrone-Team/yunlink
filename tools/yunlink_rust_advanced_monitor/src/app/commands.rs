use eframe::egui;

use super::MonitorApp;
use crate::model::{CommandLifecycle, MonitorState};
use crate::runtime_client::RuntimeCommand;

/// Render all editable command payload controls.
pub(super) fn command_inputs(app: &mut MonitorApp, ui: &mut egui::Ui) {
    ui.collapsing("TAKEOFF", |ui| {
        ui.add(egui::Slider::new(&mut app.takeoff_height_m, 0.5..=20.0).text("height m"));
        ui.add(
            egui::Slider::new(&mut app.takeoff_max_velocity_mps, 0.1..=8.0).text("max velocity"),
        );
        if ui.button("Send Takeoff").clicked() {
            app.client.send(RuntimeCommand::Takeoff {
                height_m: app.takeoff_height_m,
                max_velocity_mps: app.takeoff_max_velocity_mps,
            });
        }
    });
    ui.collapsing("LAND / RETURN", |ui| {
        ui.add(
            egui::Slider::new(&mut app.land_max_velocity_mps, 0.1..=5.0).text("land max velocity"),
        );
        if ui.button("Send Land").clicked() {
            app.client.send(RuntimeCommand::Land {
                max_velocity_mps: app.land_max_velocity_mps,
            });
        }
        ui.add(egui::Slider::new(&mut app.return_loiter_s, 0.0..=30.0).text("return loiter s"));
        if ui.button("Send Return").clicked() {
            app.client.send(RuntimeCommand::Return {
                loiter_s: app.return_loiter_s,
            });
        }
    });
    ui.collapsing("GOTO", |ui| {
        ui.add(egui::DragValue::new(&mut app.goto_x_m).prefix("x "));
        ui.add(egui::DragValue::new(&mut app.goto_y_m).prefix("y "));
        ui.add(egui::DragValue::new(&mut app.goto_z_m).prefix("z "));
        ui.add(egui::DragValue::new(&mut app.goto_yaw_rad).prefix("yaw "));
        if ui.button("Send Goto").clicked() {
            app.client.send(RuntimeCommand::Goto {
                x_m: app.goto_x_m,
                y_m: app.goto_y_m,
                z_m: app.goto_z_m,
                yaw_rad: app.goto_yaw_rad,
            });
        }
    });
    ui.collapsing("VELOCITY", |ui| {
        ui.add(egui::DragValue::new(&mut app.velocity_vx_mps).prefix("vx "));
        ui.add(egui::DragValue::new(&mut app.velocity_vy_mps).prefix("vy "));
        ui.add(egui::DragValue::new(&mut app.velocity_vz_mps).prefix("vz "));
        ui.add(egui::DragValue::new(&mut app.velocity_yaw_rate_radps).prefix("yaw rate "));
        ui.checkbox(&mut app.velocity_body_frame, "body frame");
        if ui.button("Send Velocity").clicked() {
            app.client.send(RuntimeCommand::Velocity {
                vx_mps: app.velocity_vx_mps,
                vy_mps: app.velocity_vy_mps,
                vz_mps: app.velocity_vz_mps,
                yaw_rate_radps: app.velocity_yaw_rate_radps,
                body_frame: app.velocity_body_frame,
            });
        }
    });
}

/// Render command history with message/correlation information surfaced from C ABI.
pub(super) fn command_history(state: &MonitorState, ui: &mut egui::Ui) {
    egui::ScrollArea::vertical().show(ui, |ui| {
        egui::Grid::new("command-history-grid")
            .striped(true)
            .num_columns(7)
            .show(ui, |ui| {
                ui.strong("seq");
                ui.strong("sent");
                ui.strong("action");
                ui.strong("state");
                ui.strong("session");
                ui.strong("message");
                ui.strong("detail");
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
    ui.colored_label(color, format!("{lifecycle:?}"));
}
