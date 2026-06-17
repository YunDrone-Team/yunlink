use std::sync::mpsc;

use tokio::sync::broadcast;
use yunlink::{Event, Runtime};

use super::RuntimeUpdate;

pub(super) async fn drain_events(
    events: &mut broadcast::Receiver<Event>,
    update_tx: &mpsc::Sender<RuntimeUpdate>,
) {
    loop {
        match events.try_recv() {
            Ok(event) => {
                let _ = update_tx.send(RuntimeUpdate::Event(event));
            }
            Err(broadcast::error::TryRecvError::Empty) => break,
            Err(broadcast::error::TryRecvError::Lagged(skipped)) => {
                // Backpressure is part of the Rust adapter contract. Surface it
                // visibly instead of hiding it behind an automatic recovery path.
                let _ = update_tx.send(RuntimeUpdate::Note(format!(
                    "event receiver lagged by {skipped}"
                )));
            }
            Err(broadcast::error::TryRecvError::Closed) => break,
        }
    }
}

pub(super) fn send_authority(runtime: &Runtime, update_tx: &mpsc::Sender<RuntimeUpdate>) {
    match runtime.current_authority() {
        Ok(Some(lease)) => {
            let _ = update_tx.send(RuntimeUpdate::Authority {
                state: format!("{:?}", lease.state),
            });
        }
        Ok(None) => {
            let _ = update_tx.send(RuntimeUpdate::Authority {
                state: "None".to_string(),
            });
        }
        Err(err) => send_error(update_tx, err),
    }
}

/// Convert a successful publish return value into a UI history update.
pub(super) fn send_command_sent(
    update_tx: &mpsc::Sender<RuntimeUpdate>,
    action: &str,
    detail: String,
    handle: yunlink::CommandHandle,
) {
    let _ = update_tx.send(RuntimeUpdate::CommandSent {
        action: action.to_string(),
        detail,
        handle,
    });
}

/// Collapse typed SDK errors into display text for the prototype UI.
pub(super) fn send_error(update_tx: &mpsc::Sender<RuntimeUpdate>, err: yunlink::YunlinkError) {
    let _ = update_tx.send(RuntimeUpdate::Error(err.to_string()));
}
