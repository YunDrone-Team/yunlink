use eframe::egui;

use crate::model::StateSnapshotRow;

/// Render one state snapshot row.
pub(super) fn state_row(ui: &mut egui::Ui, row: &StateSnapshotRow) {
    ui.monospace(&row.key);
    ui.label(&row.value);
    ui.monospace(row.updated_at_ms.to_string());
}

/// Render a compact label/value pair for the status banner.
pub(super) fn label_value(ui: &mut egui::Ui, label: &str, value: &str) {
    ui.strong(label);
    ui.label(if value.is_empty() { "-" } else { value });
}

/// Convert a boolean runtime condition into a monitor-style status label.
pub(super) fn bool_label(value: bool) -> &'static str {
    if value {
        "就绪 (READY)"
    } else {
        "等待 (WAIT)"
    }
}

/// Render unset numeric identifiers as a dash.
pub(super) fn value_or_dash(value: u64) -> String {
    if value == 0 {
        "-".to_string()
    } else {
        value.to_string()
    }
}
