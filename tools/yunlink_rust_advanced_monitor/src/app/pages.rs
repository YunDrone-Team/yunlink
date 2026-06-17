use eframe::egui;

use super::commands::{command_history, command_inputs};
use super::widgets::{bool_label, label_value, state_row, value_or_dash};
use super::MonitorApp;
use crate::ffi_explain;
use crate::model::LogLevel;
use crate::runtime_client::RuntimeCommand;

impl MonitorApp {
    /// Render the compact runtime/session/authority status banner.
    pub(super) fn top_status(&self, ui: &mut egui::Ui) {
        egui::Grid::new("status-grid")
            .num_columns(4)
            .spacing([20.0, 6.0])
            .show(ui, |ui| {
                label_value(ui, "runtime", bool_label(self.state.runtime_started));
                label_value(ui, "peer", bool_label(self.state.peer_ready));
                label_value(ui, "session", &value_or_dash(self.state.session_id));
                label_value(ui, "authority", &self.state.authority_state);
                ui.end_row();
                label_value(
                    ui,
                    "remote",
                    &format!("{}:{}", self.config.remote_ip, self.config.remote_tcp_port),
                );
                label_value(ui, "listen", &self.config.tcp_listen_port.to_string());
                label_value(
                    ui,
                    "agent",
                    &format!("{}#{}", self.config.agent_name, self.config.agent_id),
                );
                label_value(ui, "ABI", &ffi_explain::abi_version().to_string());
                ui.end_row();
            });
    }

    /// Render the command page, including connection actions and command drafts.
    pub(super) fn commands_page(&mut self, ui: &mut egui::Ui) {
        self.top_status(ui);
        ui.separator();

        ui.horizontal(|ui| {
            if ui.button("Connect").clicked() {
                self.client.send(RuntimeCommand::Connect);
            }
            if ui.button("Request Authority").clicked() {
                self.client.send(RuntimeCommand::RequestAuthority);
            }
            if ui.button("Release Authority").clicked() {
                self.client.send(RuntimeCommand::ReleaseAuthority);
            }
        });

        ui.separator();
        ui.columns(2, |columns| {
            columns[0].heading("Command Panel");
            command_inputs(self, &mut columns[0]);
            columns[1].heading("Command History");
            command_history(&self.state, &mut columns[1]);
        });
    }

    /// Render the System Service page placeholder.
    pub(super) fn system_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("System Service");
        ui.label("This Rust monitor shell keeps the page shape of the C++ Advanced Monitor.");
        ui.separator();
        ui.label("Current status: the public C ABI does not yet expose system service request/response helpers.");
        ui.label("Planned safe APIs: list_features, get_feature, start_feature, stop_feature.");
        ui.add_space(8.0);
        ui.group(|ui| {
            ui.label("Translation target");
            ui.monospace("Runtime::list_features -> yunlink_sys::yunlink_system_service_list_features -> yunlink_system_service_list_features");
        });
    }

    /// Render state snapshots currently exposed by the C ABI.
    pub(super) fn state_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("State");
        ui.label("VehicleCoreState is live through the current C ABI. Rich Sunray snapshots remain an ABI extension item.");
        ui.separator();
        if self.state.state_rows.is_empty() {
            ui.label("WAIT");
            return;
        }
        egui::Grid::new("state-grid")
            .striped(true)
            .num_columns(3)
            .show(ui, |ui| {
                ui.strong("field");
                ui.strong("value");
                ui.strong("updated_at_ms");
                ui.end_row();
                for row in &self.state.state_rows {
                    state_row(ui, row);
                    ui.end_row();
                }
            });
    }

    /// Render runtime logs collected from worker updates and SDK events.
    pub(super) fn logs_page(&mut self, ui: &mut egui::Ui) {
        ui.horizontal(|ui| {
            ui.checkbox(&mut self.show_info_logs, "show info");
            if ui.button("Clear").clicked() {
                self.state.logs.clear();
            }
        });
        ui.separator();
        egui::ScrollArea::vertical().show(ui, |ui| {
            for entry in &self.state.logs {
                if !self.show_info_logs && entry.level == LogLevel::Info {
                    continue;
                }
                ui.horizontal_wrapped(|ui| {
                    ui.monospace(format!("#{} {}", entry.sequence, entry.timestamp_ms));
                    ui.label(format!("{:?}", entry.level));
                    ui.label(entry.source);
                    ui.label(&entry.message);
                });
            }
        });
    }

    /// Render the teaching page that explains the ABI translation path.
    pub(super) fn abi_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("Rust -> C ABI -> C++ Core");
        ui.label("The monitor business code calls the safe `yunlink` crate. The raw `yunlink-sys` layer is shown here as an explanation surface.");
        ui.separator();
        ui.label(format!(
            "Loaded ABI version: {}",
            ffi_explain::abi_version()
        ));
        ui.add_space(8.0);
        egui::Grid::new("abi-mapping-grid")
            .striped(true)
            .num_columns(4)
            .show(ui, |ui| {
                ui.strong("safe Rust API");
                ui.strong("yunlink-sys symbol");
                ui.strong("C ABI symbol");
                ui.strong("C structs");
                ui.end_row();
                for mapping in ffi_explain::mappings() {
                    ui.monospace(mapping.safe_api);
                    ui.monospace(mapping.sys_symbol);
                    ui.monospace(mapping.c_abi);
                    ui.monospace(mapping.c_structs);
                    ui.end_row();
                }
            });
        ui.add_space(12.0);
        ui.heading("Struct Shape");
        for (name, detail) in ffi_explain::struct_examples() {
            ui.horizontal_wrapped(|ui| {
                ui.monospace(*name);
                ui.label(*detail);
            });
        }
    }
}
