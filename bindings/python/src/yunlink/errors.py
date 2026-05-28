from __future__ import annotations

from typing import Any


class YunlinkError(RuntimeError):
    def __init__(self, code_name: str):
        super().__init__(code_name)
        self.code_name = code_name


class InvalidArgumentError(YunlinkError):
    pass


class ConnectError(YunlinkError):
    pass


class InvalidStateError(YunlinkError):
    pass


class NotFoundError(YunlinkError):
    pass


_ERROR_TYPES: dict[str, type[YunlinkError]] = {
    "YUNLINK_RESULT_INVALID_ARGUMENT": InvalidArgumentError,
    "YUNLINK_RESULT_CONNECT_ERROR": ConnectError,
    "YUNLINK_RESULT_INVALID_STATE": InvalidStateError,
    "YUNLINK_RESULT_NOT_FOUND": NotFoundError,
}


def wrap_native_error(exc: RuntimeError) -> YunlinkError:
    code_name = str(exc)
    error_type = _ERROR_TYPES.get(code_name, YunlinkError)
    return error_type(code_name)


def call_native(fn: Any, *args: Any) -> Any:
    try:
        return fn(*args)
    except RuntimeError as exc:
        raise wrap_native_error(exc) from exc
