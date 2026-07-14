use std::collections::VecDeque;
use std::time::{SystemTime, UNIX_EPOCH};

use yunlink::{
    CommandHandle, CommandKind, CommandPhase, CommandResultEvent, Event, VehicleCoreStateEvent,
};

use super::{CommandLifecycle, LogLevel};

/// One UI log line.
#[derive(Debug, Clone)]
pub struct LogEntry {
    /// Monotonic sequence number assigned by the UI model.
    pub sequence: u64,
    /// Wall-clock timestamp in milliseconds.
    pub timestamp_ms: u64,
    /// Severity used for filtering and display color.
    pub level: LogLevel,
    /// Short domain label such as Runtime, Link, or Command.
    pub source: &'static str,
    /// Human-readable message.
    pub message: String,
}

/// One row in the command history table.
#[derive(Debug, Clone)]
pub struct CommandHistoryEntry {
    /// Local UI sequence number.
    pub sequence: u64,
    /// Time the command was sent from the monitor.
    pub sent_at_ms: u64,
    /// Time the row last changed.
    pub updated_at_ms: u64,
    /// Session id returned by the protocol runtime.
    pub session_id: u64,
    /// Message id returned in `yunlink_command_handle_t`.
    pub message_id: u64,
    /// Correlation id used to match command results to command sends.
    pub correlation_id: u64,
    /// Human-readable command name.
    pub action: String,
    /// Payload summary captured at send time.
    pub detail: String,
    /// Current UI lifecycle.
    pub lifecycle: CommandLifecycle,
    /// Last command result phase label.
    pub phase: String,
    /// Last command result detail string.
    pub result_detail: String,
}

/// One display row for the State page.
#[derive(Debug, Clone)]
pub struct StateSnapshotRow {
    /// Field key.
    pub key: String,
    /// Rendered field value.
    pub value: String,
    /// Time this value was observed by the UI model.
    pub updated_at_ms: u64,
}

/// Mutable UI snapshot consumed by egui rendering.
#[derive(Debug, Default)]
pub struct MonitorState {
    /// Whether the local runtime started.
    pub runtime_started: bool,
    /// Whether a peer appears connected.
    pub peer_ready: bool,
    /// Rendered authority state.
    pub authority_state: String,
    /// Last known peer id.
    pub peer_id: String,
    /// Last opened session id.
    pub session_id: u64,
    /// Rendered session state.
    pub session_state: String,
    /// Most recent error.
    pub last_error: String,
    /// Most recent non-error note.
    pub last_note: String,
    /// Most recent commands, newest first.
    pub command_history: VecDeque<CommandHistoryEntry>,
    /// Most recent logs, newest first.
    pub logs: VecDeque<LogEntry>,
    /// Current state rows.
    pub state_rows: Vec<StateSnapshotRow>,
    next_log_sequence: u64,
    next_command_sequence: u64,
}

impl MonitorState {
    /// Append a log entry and enforce the UI log retention limit.
    pub fn push_log(&mut self, level: LogLevel, source: &'static str, message: impl Into<String>) {
        self.next_log_sequence += 1;
        self.logs.push_front(LogEntry {
            sequence: self.next_log_sequence,
            timestamp_ms: now_ms(),
            level,
            source,
            message: message.into(),
        });
        truncate(&mut self.logs, 500);
    }

    /// Add a command row after a safe SDK publish call returns a command handle.
    pub fn record_command(&mut self, action: &str, detail: String, handle: CommandHandle) {
        self.next_command_sequence += 1;
        self.command_history.push_front(CommandHistoryEntry {
            sequence: self.next_command_sequence,
            sent_at_ms: now_ms(),
            updated_at_ms: now_ms(),
            session_id: handle.session_id,
            message_id: handle.message_id,
            correlation_id: handle.correlation_id,
            action: action.to_string(),
            detail,
            lifecycle: CommandLifecycle::Sent,
            phase: "sent".to_string(),
            result_detail: String::new(),
        });
        truncate(&mut self.command_history, 64);
    }

    /// Apply a safe SDK runtime event to the render model.
    pub fn apply_event(&mut self, event: Event) {
        match event {
            Event::Link(link) => {
                self.peer_ready = link.is_up;
                self.peer_id = link.peer_id;
                self.push_log(
                    LogLevel::Info,
                    "链路",
                    format!("链路状态 is_up={}", link.is_up),
                );
            }
            Event::Error(error) => {
                self.last_error = error.message.clone();
                self.push_log(LogLevel::Error, "运行时", error.message);
            }
            Event::CommandResult(result) => self.apply_command_result(result),
            Event::VehicleCoreState(state) => self.apply_vehicle_state(state),
        }
    }

    /// Match a command-result event to an existing command row by correlation id.
    fn apply_command_result(&mut self, result: CommandResultEvent) {
        let mut found = false;
        for entry in &mut self.command_history {
            if entry.correlation_id == result.correlation_id {
                entry.updated_at_ms = now_ms();
                entry.lifecycle = lifecycle_from_phase(result.phase);
                entry.phase = format!("{:?}", result.phase);
                entry.result_detail = result.detail.clone();
                found = true;
                break;
            }
        }
        if !found {
            self.record_unmatched_command_result(result);
        }
    }

    fn record_unmatched_command_result(&mut self, result: CommandResultEvent) {
        self.next_command_sequence += 1;
        self.command_history.push_front(CommandHistoryEntry {
            sequence: self.next_command_sequence,
            sent_at_ms: now_ms(),
            updated_at_ms: now_ms(),
            session_id: result.session_id,
            message_id: result.message_id,
            correlation_id: result.correlation_id,
            action: command_kind_label(result.command_kind).to_string(),
            detail: "收到结果，但本地没有对应发送记录".to_string(),
            lifecycle: lifecycle_from_phase(result.phase),
            phase: format!("{:?}", result.phase),
            result_detail: result.detail.clone(),
        });
    }

    /// Convert the current VehicleCoreState event into simple display rows.
    fn apply_vehicle_state(&mut self, state: VehicleCoreStateEvent) {
        self.state_rows = vec![
            row("session_id", state.session_id.to_string()),
            row("message_id", state.message_id.to_string()),
            row("armed", state.armed.to_string()),
            row("battery_percent", format!("{:.1}", state.battery_percent)),
        ];
    }
}

/// Trim a newest-first queue to a fixed limit.
fn truncate<T>(items: &mut VecDeque<T>, limit: usize) {
    while items.len() > limit {
        items.pop_back();
    }
}

/// Build a state display row with the current observation timestamp.
fn row(key: &str, value: String) -> StateSnapshotRow {
    StateSnapshotRow {
        key: key.to_string(),
        value,
        updated_at_ms: now_ms(),
    }
}

/// Map protocol command phases into monitor-level lifecycle buckets.
fn lifecycle_from_phase(phase: CommandPhase) -> CommandLifecycle {
    match phase {
        CommandPhase::Succeeded => CommandLifecycle::Succeeded,
        CommandPhase::Failed => CommandLifecycle::Failed,
        CommandPhase::Cancelled => CommandLifecycle::Cancelled,
        CommandPhase::Expired => CommandLifecycle::Expired,
        _ => CommandLifecycle::Received,
    }
}

/// Render command kinds with compact monitor labels.
fn command_kind_label(kind: CommandKind) -> &'static str {
    match kind {
        CommandKind::Takeoff => "TAKEOFF",
        CommandKind::Land => "LAND",
        CommandKind::Return => "RETURN",
        CommandKind::Goto => "GOTO",
        CommandKind::VelocitySetpoint => "VELOCITY",
        CommandKind::TrajectoryChunk => "TRAJECTORY",
        CommandKind::FormationTask => "FORMATION",
        CommandKind::Unknown | CommandKind::Other(_) => "UNKNOWN",
    }
}

/// Wall-clock timestamp helper for UI rows.
fn now_ms() -> u64 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map(|value| value.as_millis() as u64)
        .unwrap_or(0)
}
