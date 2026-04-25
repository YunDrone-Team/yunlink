from __future__ import annotations

import asyncio
import queue
import threading
import time
from dataclasses import dataclass
from enum import IntEnum
from typing import Any

from ._yunlink_native import RuntimeCore


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


def _wrap_native_error(exc: RuntimeError) -> YunlinkError:
    code_name = str(exc)
    error_type = _ERROR_TYPES.get(code_name, YunlinkError)
    return error_type(code_name)


def _call_native(fn: Any, *args: Any) -> Any:
    try:
        return fn(*args)
    except RuntimeError as exc:
        raise _wrap_native_error(exc) from exc


class AgentType(IntEnum):
    GROUND_STATION = 1
    UAV = 2


class ControlSource(IntEnum):
    GROUND_STATION = 1


@dataclass(frozen=True)
class RuntimeConfig:
    udp_bind_port: int
    udp_target_port: int
    tcp_listen_port: int
    agent_type: AgentType
    agent_id: int

    def to_native(self) -> dict[str, int]:
        role = 2 if self.agent_type == AgentType.GROUND_STATION else 3
        return {
            "udp_bind_port": self.udp_bind_port,
            "udp_target_port": self.udp_target_port,
            "tcp_listen_port": self.tcp_listen_port,
            "agent_type": int(self.agent_type),
            "agent_id": self.agent_id,
            "role": role,
        }


@dataclass(frozen=True)
class PeerConnection:
    id: str


@dataclass(frozen=True)
class Session:
    session_id: int


@dataclass(frozen=True)
class TargetSelector:
    scope: int
    target_type: int
    entity_id: int
    group_id: int = 0

    @staticmethod
    def entity(agent_type: AgentType, entity_id: int) -> "TargetSelector":
        return TargetSelector(1, int(agent_type), entity_id, 0)

    @staticmethod
    def broadcast(agent_type: AgentType) -> "TargetSelector":
        return TargetSelector(3, int(agent_type), 0, 0)

    def to_native(self) -> dict[str, int]:
        return {
            "scope": self.scope,
            "target_type": self.target_type,
            "entity_id": self.entity_id,
            "group_id": self.group_id,
        }


@dataclass(frozen=True)
class GotoCommand:
    x_m: float
    y_m: float
    z_m: float
    yaw_rad: float

    def to_native(self) -> dict[str, float]:
        return {
            "x_m": self.x_m,
            "y_m": self.y_m,
            "z_m": self.z_m,
            "yaw_rad": self.yaw_rad,
        }


@dataclass(frozen=True)
class VehicleCoreState:
    armed: bool
    nav_mode: int
    x_m: float
    y_m: float
    z_m: float
    vx_mps: float
    vy_mps: float
    vz_mps: float
    battery_percent: float

    def to_native(self) -> dict[str, Any]:
        return {
            "armed": self.armed,
            "nav_mode": self.nav_mode,
            "x_m": self.x_m,
            "y_m": self.y_m,
            "z_m": self.z_m,
            "vx_mps": self.vx_mps,
            "vy_mps": self.vy_mps,
            "vz_mps": self.vz_mps,
            "battery_percent": self.battery_percent,
        }


@dataclass(frozen=True)
class CommandHandle:
    session_id: int
    message_id: int
    correlation_id: int


@dataclass(frozen=True)
class AuthorityLease:
    state: int
    session_id: int
    peer: PeerConnection


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


class Runtime:
    def __init__(self, core: RuntimeCore):
        self._core = core
        self._stop = threading.Event()
        self._poll_error: YunlinkError | None = None
        self._subscribers: list[queue.Queue] = []
        self._async_subscribers: list[tuple[asyncio.AbstractEventLoop, asyncio.Queue]] = []
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.start()

    @classmethod
    def start(cls, config: RuntimeConfig) -> "Runtime":
        core = _call_native(RuntimeCore)
        _call_native(core.start, config.to_native())
        return cls(core)

    def close(self) -> None:
        if self._stop.is_set():
            return
        self._stop.set()
        self._thread.join(timeout=1.0)
        _call_native(self._core.stop)

    def subscribe(self) -> queue.Queue:
        q: queue.Queue = queue.Queue()
        self._subscribers.append(q)
        return q

    def subscribe_async(self) -> asyncio.Queue:
        q: asyncio.Queue = asyncio.Queue()
        loop = asyncio.get_running_loop()
        self._async_subscribers.append((loop, q))
        return q

    def last_poll_error(self) -> YunlinkError | None:
        return self._poll_error

    def connect(self, ip: str, port: int) -> PeerConnection:
        return PeerConnection(_call_native(self._core.connect, ip, port))

    async def connect_async(self, ip: str, port: int) -> PeerConnection:
        return await asyncio.to_thread(self.connect, ip, port)

    def open_session(self, peer: PeerConnection, node_name: str) -> Session:
        return Session(_call_native(self._core.open_session, peer.id, node_name))

    async def open_session_async(self, peer: PeerConnection, node_name: str) -> Session:
        return await asyncio.to_thread(self.open_session, peer, node_name)

    def request_authority(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        source: ControlSource,
        lease_ttl_ms: int,
        allow_preempt: bool,
    ) -> None:
        _call_native(
            self._core.request_authority,
            peer.id,
            session.session_id,
            target.to_native(),
            int(source),
            lease_ttl_ms,
            allow_preempt,
        )

    async def request_authority_async(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        source: ControlSource,
        lease_ttl_ms: int,
        allow_preempt: bool,
    ) -> None:
        await asyncio.to_thread(
            self.request_authority,
            peer,
            session,
            target,
            source,
            lease_ttl_ms,
            allow_preempt,
        )

    def release_authority(
        self, peer: PeerConnection, session: Session, target: TargetSelector
    ) -> None:
        _call_native(
            self._core.release_authority, peer.id, session.session_id, target.to_native()
        )

    async def release_authority_async(
        self, peer: PeerConnection, session: Session, target: TargetSelector
    ) -> None:
        await asyncio.to_thread(self.release_authority, peer, session, target)

    def renew_authority(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        source: ControlSource,
        lease_ttl_ms: int,
    ) -> None:
        _call_native(
            self._core.renew_authority,
            peer.id,
            session.session_id,
            target.to_native(),
            int(source),
            lease_ttl_ms,
        )

    async def renew_authority_async(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        source: ControlSource,
        lease_ttl_ms: int,
    ) -> None:
        await asyncio.to_thread(
            self.renew_authority, peer, session, target, source, lease_ttl_ms
        )

    def current_authority(self) -> AuthorityLease | None:
        lease = _call_native(self._core.current_authority)
        if lease is None:
            return None
        return AuthorityLease(
            state=lease["state"],
            session_id=lease["session_id"],
            peer=PeerConnection(lease["peer_id"]),
        )

    def publish_goto(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        payload: GotoCommand,
    ) -> CommandHandle:
        result = _call_native(
            self._core.publish_goto,
            peer.id, session.session_id, target.to_native(), payload.to_native()
        )
        return CommandHandle(
            session_id=result["session_id"],
            message_id=result["message_id"],
            correlation_id=result["correlation_id"],
        )

    async def publish_goto_async(
        self,
        peer: PeerConnection,
        session: Session,
        target: TargetSelector,
        payload: GotoCommand,
    ) -> CommandHandle:
        return await asyncio.to_thread(self.publish_goto, peer, session, target, payload)

    def publish_vehicle_core_state(
        self,
        peer: PeerConnection,
        target: TargetSelector,
        payload: VehicleCoreState,
        session_id: int,
    ) -> None:
        _call_native(
            self._core.publish_vehicle_core_state,
            peer.id, target.to_native(), payload.to_native(), session_id
        )

    async def publish_vehicle_core_state_async(
        self,
        peer: PeerConnection,
        target: TargetSelector,
        payload: VehicleCoreState,
        session_id: int,
    ) -> None:
        await asyncio.to_thread(
            self.publish_vehicle_core_state, peer, target, payload, session_id
        )

    def _poll_loop(self) -> None:
        while not self._stop.is_set():
            try:
                event = _call_native(self._core.poll_event)
            except YunlinkError as exc:
                self._poll_error = exc
                parsed = ErrorEvent(-1, str(exc))
                for subscriber in list(self._subscribers):
                    subscriber.put(parsed)
                for loop, subscriber in list(self._async_subscribers):
                    loop.call_soon_threadsafe(subscriber.put_nowait, parsed)
                break
            if event is None:
                time.sleep(0.01)
                continue

            parsed = _coerce_event(event)
            if parsed is None:
                continue

            for subscriber in list(self._subscribers):
                subscriber.put(parsed)
            for loop, subscriber in list(self._async_subscribers):
                loop.call_soon_threadsafe(subscriber.put_nowait, parsed)


def _coerce_event(event: dict[str, Any]) -> object | None:
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
    "RuntimeConfig",
    "Session",
    "TargetSelector",
    "VehicleCoreState",
    "VehicleCoreStateEvent",
    "YunlinkError",
]
