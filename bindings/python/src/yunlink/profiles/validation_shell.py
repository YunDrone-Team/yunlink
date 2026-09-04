MAX_SHELL_INPUT_BYTES = 16 * 1024


def _valid_window(cols: int, rows: int) -> bool:
    return 20 <= cols <= 400 and 5 <= rows <= 200


def validate_shell_open_request(request) -> None:
    if not _valid_window(request.cols, request.rows):
        raise ValueError("invalid shell window")


def validate_shell_write_request(request) -> None:
    if not request.session_uid or len(request.data) > MAX_SHELL_INPUT_BYTES:
        raise ValueError("invalid shell write request")


def validate_shell_resize_request(request) -> None:
    if not request.session_uid or not _valid_window(request.cols, request.rows):
        raise ValueError("invalid shell resize request")


def validate_shell_close_request(request) -> None:
    if not request.session_uid:
        raise ValueError("shell session UID is required")
