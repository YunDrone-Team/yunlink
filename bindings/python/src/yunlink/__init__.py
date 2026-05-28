from __future__ import annotations

from ._yunlink_native import RuntimeCore
from .errors import (
    ConnectError,
    InvalidArgumentError,
    InvalidStateError,
    NotFoundError,
    YunlinkError,
    wrap_native_error as _wrap_native_error,
)
from .events import (
    CommandResultEvent,
    ErrorEvent,
    LinkEvent,
    VehicleCoreStateEvent,
    coerce_event as _coerce_event,
)
from .runtime import Runtime
from .types import (
    AgentType,
    AuthorityLease,
    CommandHandle,
    ControlSource,
    GotoCommand,
    PeerConnection,
    RuntimeConfig,
    Session,
    TargetSelector,
    VehicleCoreState,
)

__all__ = [
    "AgentType",
    "AuthorityLease",
    "CommandHandle",
    "CommandResultEvent",
    "ConnectError",
    "ControlSource",
    "ErrorEvent",
    "GotoCommand",
    "InvalidArgumentError",
    "InvalidStateError",
    "LinkEvent",
    "NotFoundError",
    "PeerConnection",
    "Runtime",
    "RuntimeCore",
    "RuntimeConfig",
    "Session",
    "TargetSelector",
    "VehicleCoreState",
    "VehicleCoreStateEvent",
    "YunlinkError",
]
