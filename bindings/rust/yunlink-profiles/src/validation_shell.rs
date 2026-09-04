use crate::shell::{ShellCloseRequest, ShellOpenRequest, ShellResizeRequest, ShellWriteRequest};

const MAX_INPUT_BYTES: usize = 16 * 1024;

fn valid_window(cols: u32, rows: u32) -> bool {
    (20..=400).contains(&cols) && (5..=200).contains(&rows)
}

pub fn validate_shell_open_request(request: &ShellOpenRequest) -> Result<(), &'static str> {
    valid_window(request.cols, request.rows).then_some(()).ok_or("invalid shell window")
}

pub fn validate_shell_write_request(request: &ShellWriteRequest) -> Result<(), &'static str> {
    (!request.session_uid.is_empty() && request.data.len() <= MAX_INPUT_BYTES)
        .then_some(())
        .ok_or("invalid shell write request")
}

pub fn validate_shell_resize_request(request: &ShellResizeRequest) -> Result<(), &'static str> {
    (!request.session_uid.is_empty() && valid_window(request.cols, request.rows))
        .then_some(())
        .ok_or("invalid shell resize request")
}

pub fn validate_shell_close_request(request: &ShellCloseRequest) -> Result<(), &'static str> {
    (!request.session_uid.is_empty())
        .then_some(())
        .ok_or("shell session UID is required")
}
