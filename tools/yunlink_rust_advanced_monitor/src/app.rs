//! egui application shell for the Rust Advanced Monitor prototype.
//!
//! This module is intentionally UI-only. It does not import `yunlink-sys`, does
//! not handle raw C pointers, and does not know about C struct layout. Runtime
//! work is delegated to `RuntimeClient`, and ABI teaching content is read from
//! `ffi_explain`.

mod commands;
mod pages;
mod widgets;

use eframe::egui;

use crate::model::{LogLevel, MonitorConfig, MonitorPage, MonitorState};
use crate::runtime_client::{RuntimeClient, RuntimeCommand, RuntimeUpdate};

#[derive(Debug, Clone, Copy)]
pub(super) enum PendingHighRiskCommand {
    Takeoff,
    Land,
    Return,
}

/// Top-level egui application state.
///
/// The fields are split into three groups: static launch configuration, UI
/// navigation/display state, and editable command payload drafts.
pub struct MonitorApp {
    /// Runtime and network options parsed from CLI arguments.
    config: MonitorConfig,
    /// Currently selected left-nav page.
    page: MonitorPage,
    /// UI-ready snapshot of runtime/session/command/state/log information.
    state: MonitorState,
    /// Channel-backed handle to the background safe-SDK runtime worker.
    client: RuntimeClient,
    /// High-risk action waiting for explicit operator confirmation.
    pending_high_risk: Option<PendingHighRiskCommand>,
    /// Draft goto X coordinate.
    goto_x_m: f32,
    /// Draft goto Y coordinate.
    goto_y_m: f32,
    /// Draft goto Z coordinate.
    goto_z_m: f32,
    /// Draft goto yaw.
    goto_yaw_rad: f32,
    /// Draft velocity X component.
    velocity_vx_mps: f32,
    /// Draft velocity Y component.
    velocity_vy_mps: f32,
    /// Draft velocity Z component.
    velocity_vz_mps: f32,
    /// Draft velocity yaw-rate component.
    velocity_yaw_rate_radps: f32,
    /// Draft frame selector for velocity commands.
    velocity_body_frame: bool,
    /// Log filter toggle.
    show_info_logs: bool,
}

impl MonitorApp {
    /// Create the egui app and immediately start the runtime worker.
    pub fn new(config: MonitorConfig) -> Self {
        let client = RuntimeClient::spawn(config.clone());
        Self {
            config,
            page: MonitorPage::Commands,
            state: MonitorState::default(),
            client,
            pending_high_risk: None,
            goto_x_m: 5.0,
            goto_y_m: 0.0,
            goto_z_m: 2.0,
            goto_yaw_rad: 0.0,
            velocity_vx_mps: 0.0,
            velocity_vy_mps: 0.0,
            velocity_vz_mps: 0.0,
            velocity_yaw_rate_radps: 0.0,
            velocity_body_frame: true,
            show_info_logs: true,
        }
    }

    /// Apply all worker updates that arrived since the previous UI frame.
    ///
    /// This keeps the UI model single-threaded even though the underlying
    /// protocol runtime runs on a background thread.
    fn drain_runtime_updates(&mut self) {
        for update in self.client.drain_updates() {
            match update {
                RuntimeUpdate::Started => {
                    self.state.runtime_started = true;
                    self.state
                        .push_log(LogLevel::Info, "运行时", "运行时已通过安全 Rust SDK 启动");
                }
                RuntimeUpdate::Connected {
                    peer_id,
                    session_id,
                } => {
                    self.state.peer_ready = true;
                    self.state.peer_id = peer_id;
                    self.state.session_id = session_id;
                    self.state.session_state = "OPENED".to_string();
                    self.state
                        .push_log(LogLevel::Info, "连接", "peer 已连接，session 已建立");
                }
                RuntimeUpdate::Authority { state } => {
                    self.state.authority_state = state;
                    self.state
                        .push_log(LogLevel::Info, "控制权", "控制权状态已更新");
                }
                RuntimeUpdate::CommandSent {
                    action,
                    detail,
                    handle,
                } => {
                    self.state.record_command(&action, detail, handle);
                    self.state.push_log(LogLevel::Info, "命令", action);
                }
                RuntimeUpdate::Event(event) => self.state.apply_event(event),
                RuntimeUpdate::Error(error) => {
                    self.state.last_error = error.clone();
                    self.state.push_log(LogLevel::Error, "运行时", error);
                }
                RuntimeUpdate::Note(note) => {
                    self.state.last_note = note.clone();
                    self.state.push_log(LogLevel::Warn, "运行时", note);
                }
            }
        }
    }

    /// Render a page navigation button and update the selected page on click.
    fn nav_button(&mut self, ui: &mut egui::Ui, page: MonitorPage, label: &str) {
        let selected = self.page == page;
        if ui.selectable_label(selected, label).clicked() {
            self.page = page;
        }
    }
}

impl eframe::App for MonitorApp {
    /// Render one egui frame.
    fn ui(&mut self, ui: &mut egui::Ui, _frame: &mut eframe::Frame) {
        self.drain_runtime_updates();
        egui::Panel::left("nav")
            .resizable(false)
            .exact_size(180.0)
            .show_inside(ui, |ui| {
                ui.heading("YunLink");
                ui.label("Rust 监视器");
                ui.separator();
                self.nav_button(ui, MonitorPage::Commands, "控制");
                self.nav_button(ui, MonitorPage::System, "系统服务");
                self.nav_button(ui, MonitorPage::State, "状态");
                self.nav_button(ui, MonitorPage::Logs, "日志");
                self.nav_button(ui, MonitorPage::Abi, "ABI 说明");
            });
        egui::CentralPanel::default().show_inside(ui, |ui| match self.page {
            MonitorPage::Commands => self.commands_page(ui),
            MonitorPage::System => self.system_page(ui),
            MonitorPage::State => self.state_page(ui),
            MonitorPage::Logs => self.logs_page(ui),
            MonitorPage::Abi => self.abi_page(ui),
        });
        ui.ctx()
            .request_repaint_after(std::time::Duration::from_millis(100));
    }
}

impl Drop for MonitorApp {
    fn drop(&mut self) {
        // Ask the worker to exit before the egui app is dropped. The worker owns
        // the safe `yunlink::Runtime`, whose Drop path closes the C ABI runtime.
        self.client.send(RuntimeCommand::Shutdown);
    }
}
