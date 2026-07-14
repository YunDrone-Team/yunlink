use eframe::egui;

use super::commands::{command_history, command_inputs, high_risk_confirmation};
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
                label_value(ui, "运行时", bool_label(self.state.runtime_started));
                label_value(ui, "peer", bool_label(self.state.peer_ready));
                label_value(ui, "session", &value_or_dash(self.state.session_id));
                label_value(ui, "控制权", &authority_label(&self.state.authority_state));
                ui.end_row();
                label_value(
                    ui,
                    "对端",
                    &format!("{}:{}", self.config.remote_ip, self.config.remote_tcp_port),
                );
                label_value(ui, "本地监听", &self.config.tcp_listen_port.to_string());
                label_value(
                    ui,
                    "Agent",
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

        ui.group(|ui| {
            ui.strong("连接与控制权");
            ui.horizontal_wrapped(|ui| {
                if ui.button("连接").clicked() {
                    self.client.send(RuntimeCommand::Connect);
                }
                if ui.button("申请控制权").clicked() {
                    self.client.send(RuntimeCommand::RequestAuthority);
                }
                if ui.button("释放控制权").clicked() {
                    self.client.send(RuntimeCommand::ReleaseAuthority);
                }
                if self.state.session_id == 0 {
                    ui.colored_label(
                        egui::Color32::from_rgb(138, 90, 0),
                        "尚未建立 active session，飞行指令已禁用",
                    );
                }
            });
        });

        ui.separator();
        ui.heading("飞行控制");
        command_inputs(self, ui);
        ui.separator();
        ui.heading("命令历史 / ACK 审计");
        command_history(&self.state, ui);
        high_risk_confirmation(self, ui.ctx());
    }

    /// Render the System Service page placeholder.
    pub(super) fn system_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("系统服务");
        ui.label("Rust 监视器保持与 C++ AdvancedMonitor 一致的页面结构。");
        ui.separator();
        ui.label("当前状态：公共 C ABI 尚未开放系统服务请求与响应辅助接口。");
        ui.label("计划提供的安全 API：list_features、get_feature、start_feature、stop_feature。");
        ui.add_space(8.0);
        ui.group(|ui| {
            ui.label("调用链目标");
            ui.monospace("Runtime::list_features -> yunlink_sys::yunlink_system_service_list_features -> yunlink_system_service_list_features");
        });
    }

    /// Render state snapshots currently exposed by the C ABI.
    pub(super) fn state_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("状态");
        ui.label("VehicleCoreState 已通过当前 C ABI 实时接入；完整 Sunray 快照仍属于 ABI 扩展项。");
        ui.separator();
        if self.state.state_rows.is_empty() {
            ui.label("等待数据 (WAIT)");
            return;
        }
        egui::Grid::new("state-grid")
            .striped(true)
            .num_columns(3)
            .show(ui, |ui| {
                ui.strong("字段");
                ui.strong("值");
                ui.strong("更新时间 (updated_at_ms)");
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
            ui.checkbox(&mut self.show_info_logs, "显示 INFO");
            if ui.button("清空日志").clicked() {
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
                    ui.label(log_level_label(entry.level));
                    ui.label(entry.source);
                    ui.label(&entry.message);
                });
            }
        });
    }

    /// Render the teaching page that explains the ABI translation path.
    pub(super) fn abi_page(&mut self, ui: &mut egui::Ui) {
        ui.heading("Rust -> C ABI -> C++ Core");
        ui.label(
            "监视器业务代码调用安全的 `yunlink` crate；此页仅展示底层 `yunlink-sys` 调用关系。",
        );
        ui.separator();
        ui.label(format!("已加载 ABI 版本：{}", ffi_explain::abi_version()));
        ui.add_space(8.0);
        egui::Grid::new("abi-mapping-grid")
            .striped(true)
            .num_columns(4)
            .show(ui, |ui| {
                ui.strong("安全 Rust API");
                ui.strong("yunlink-sys 符号");
                ui.strong("C ABI 符号");
                ui.strong("C 结构体");
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
        ui.heading("结构体布局");
        for (name, detail) in ffi_explain::struct_examples() {
            ui.horizontal_wrapped(|ui| {
                ui.monospace(*name);
                ui.label(*detail);
            });
        }
    }
}

fn authority_label(value: &str) -> String {
    match value {
        "Controller" => "控制者 (CONTROLLER)".to_string(),
        "PendingGrant" => "等待授予 (PENDING_GRANT)".to_string(),
        "Observer" => "观察者 (OBSERVER)".to_string(),
        "Preempting" => "正在抢占 (PREEMPTING)".to_string(),
        "Revoked" => "已撤销 (REVOKED)".to_string(),
        "Released" => "已释放 (RELEASED)".to_string(),
        "Rejected" => "已拒绝 (REJECTED)".to_string(),
        "None" | "" => "无 (NONE)".to_string(),
        _ => value.to_string(),
    }
}

fn log_level_label(level: LogLevel) -> &'static str {
    match level {
        LogLevel::Info => "信息 (INFO)",
        LogLevel::Warn => "警告 (WARN)",
        LogLevel::Error => "错误 (ERROR)",
    }
}
