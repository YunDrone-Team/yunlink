from __future__ import annotations

import asyncio
import queue
import threading
import time

from ._yunlink_native import RuntimeCore
from .errors import YunlinkError, call_native
from .events import ErrorEvent, coerce_event
from .configuration import coerce_configuration_response
from .configuration_runtime import ConfigurationRuntimeMixin
from .types import (
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


class Runtime(ConfigurationRuntimeMixin):
    def __init__(self, core: RuntimeCore):
        self._core = core
        self._stop = threading.Event()
        self._poll_error: YunlinkError | None = None
        self._subscribers: list[queue.Queue] = []
        self._async_subscribers: list[tuple[asyncio.AbstractEventLoop, asyncio.Queue]] = []
        self._configuration_subscribers: list[queue.Queue] = []
        self._async_configuration_subscribers: list[
            tuple[asyncio.AbstractEventLoop, asyncio.Queue]
        ] = []
        self._thread = threading.Thread(target=self._poll_loop, daemon=True)
        self._thread.start()

    @classmethod
    def start(cls, config: RuntimeConfig) -> "Runtime":
        core = call_native(RuntimeCore)
        call_native(core.start, config.to_native())
        return cls(core)

    def close(self) -> None:
        if self._stop.is_set():
            return
        self._stop.set()
        self._thread.join(timeout=1.0)
        call_native(self._core.stop)

    def subscribe(self) -> queue.Queue:
        q: queue.Queue = queue.Queue()
        self._subscribers.append(q)
        return q

    def subscribe_async(self) -> asyncio.Queue:
        q: asyncio.Queue = asyncio.Queue()
        loop = asyncio.get_running_loop()
        self._async_subscribers.append((loop, q))
        return q

    def subscribe_configuration(self) -> queue.Queue:
        q: queue.Queue = queue.Queue()
        self._configuration_subscribers.append(q)
        return q

    def subscribe_configuration_async(self) -> asyncio.Queue:
        q: asyncio.Queue = asyncio.Queue()
        loop = asyncio.get_running_loop()
        self._async_configuration_subscribers.append((loop, q))
        return q

    def last_poll_error(self) -> YunlinkError | None:
        return self._poll_error

    def connect(self, ip: str, port: int) -> PeerConnection:
        return PeerConnection(call_native(self._core.connect, ip, port))

    async def connect_async(self, ip: str, port: int) -> PeerConnection:
        return await asyncio.to_thread(self.connect, ip, port)

    def open_session(self, peer: PeerConnection, node_name: str) -> Session:
        return Session(call_native(self._core.open_session, peer.id, node_name))

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
        call_native(
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
        call_native(
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
        call_native(
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
        lease = call_native(self._core.current_authority)
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
        result = call_native(
            self._core.publish_goto,
            peer.id,
            session.session_id,
            target.to_native(),
            payload.to_native(),
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
        call_native(
            self._core.publish_vehicle_core_state,
            peer.id,
            target.to_native(),
            payload.to_native(),
            session_id,
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
                configuration_payload = None
                poll_configuration = getattr(
                    self._core, "poll_configuration_response", None
                )
                if poll_configuration is not None:
                    configuration_payload = call_native(poll_configuration)
                event = call_native(self._core.poll_event)
            except YunlinkError as exc:
                self._poll_error = exc
                parsed = ErrorEvent(-1, str(exc))
                self._publish_event(parsed)
                break
            if configuration_payload is not None:
                configuration_response = coerce_configuration_response(
                    configuration_payload
                )
                if configuration_response is not None:
                    self._publish_configuration(configuration_response)
            if event is None and configuration_payload is None:
                time.sleep(0.01)
                continue

            if event is not None:
                parsed = coerce_event(event)
                if parsed is not None:
                    self._publish_event(parsed)

    def _publish_event(self, event: object) -> None:
        for subscriber in list(self._subscribers):
            subscriber.put(event)
        for loop, subscriber in list(self._async_subscribers):
            loop.call_soon_threadsafe(subscriber.put_nowait, event)

    def _publish_configuration(self, response: object) -> None:
        for subscriber in list(self._configuration_subscribers):
            subscriber.put(response)
        for loop, subscriber in list(self._async_configuration_subscribers):
            loop.call_soon_threadsafe(subscriber.put_nowait, response)
