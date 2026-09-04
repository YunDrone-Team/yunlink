use prost::Message;
use yunlink_profiles::{shell, validate_shell_close_request, validate_shell_open_request, validate_shell_resize_request, validate_shell_write_request};

#[test]
fn shell_requests_validate_bounds_and_required_session() {
    let open = shell::ShellOpenRequest { cols: 80, rows: 24 };
    assert_eq!(hex::encode(open.encode_to_vec()), "08501018");
    assert!(validate_shell_open_request(&open).is_ok());
    assert!(validate_shell_open_request(&shell::ShellOpenRequest { cols: 19, rows: 24 }).is_err());
    assert!(validate_shell_write_request(&shell::ShellWriteRequest { session_uid: "s".into(), data: vec![0; 16 * 1024] }).is_ok());
    assert!(validate_shell_write_request(&shell::ShellWriteRequest { session_uid: "".into(), data: vec![] }).is_err());
    assert!(validate_shell_resize_request(&shell::ShellResizeRequest { session_uid: "s".into(), cols: 400, rows: 200 }).is_ok());
    assert!(validate_shell_resize_request(&shell::ShellResizeRequest { session_uid: "s".into(), cols: 401, rows: 24 }).is_err());
    assert!(validate_shell_close_request(&shell::ShellCloseRequest { session_uid: "s".into() }).is_ok());
}
