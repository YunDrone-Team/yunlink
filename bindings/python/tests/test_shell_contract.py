import pytest

from yunlink.profiles import shell
from yunlink.profiles.validation import (
    validate_shell_close_request,
    validate_shell_open_request,
    validate_shell_resize_request,
    validate_shell_write_request,
)


def test_shell_requests_validate_bounds_and_session():
    open_request = shell.ShellOpenRequest(cols=80, rows=24)
    assert open_request.SerializeToString().hex() == "08501018"
    validate_shell_open_request(open_request)
    with pytest.raises(ValueError):
        validate_shell_open_request(shell.ShellOpenRequest(cols=19, rows=24))
    validate_shell_write_request(shell.ShellWriteRequest(session_uid="s", data=b"x"))
    with pytest.raises(ValueError):
        validate_shell_write_request(shell.ShellWriteRequest(session_uid="", data=b""))
    validate_shell_resize_request(shell.ShellResizeRequest(session_uid="s", cols=400, rows=200))
    validate_shell_close_request(shell.ShellCloseRequest(session_uid="s"))
