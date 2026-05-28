from __future__ import annotations

from dataclasses import dataclass
from typing import Any


@dataclass(frozen=True)
class CommandResultEvent:
    session_id: int
    message_id: int
    correlation_id: int
    command_kind: int
    phase: int
    result_code: int
    progress_percent: int
    detail: str


@dataclass(frozen=True)
class VehicleCoreStateEvent:
    session_id: int
    message_id: int
    correlation_id: int
    armed: bool
    battery_percent: float


@dataclass(frozen=True)
class LinkEvent:
    peer_id: str
    is_up: bool


@dataclass(frozen=True)
class ErrorEvent:
    code: int
    message: str


def coerce_event(event: dict[str, Any]) -> object | None:
    kind = event.get("type")
    if kind == "command_result":
        return CommandResultEvent(
            event["session_id"],
            event["message_id"],
            event["correlation_id"],
            event["command_kind"],
            event["phase"],
            event["result_code"],
            event["progress_percent"],
            event["detail"],
        )
    if kind == "vehicle_core_state":
        return VehicleCoreStateEvent(
            event["session_id"],
            event["message_id"],
            event["correlation_id"],
            event["armed"],
            event["battery_percent"],
        )
    if kind == "link":
        return LinkEvent(event["peer_id"], event["is_up"])
    if kind == "error":
        return ErrorEvent(event["code"], event["message"])
    return None
