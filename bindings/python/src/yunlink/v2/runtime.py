"""Runtime lifecycle and message operations for ABI 2."""

from __future__ import annotations

import ctypes
import queue
from collections.abc import Sequence

from .ffi import (
    _BytesView,
    _Callback,
    _Event,
    _Handle,
    _Peer,
    _ProfileView,
    _RuntimeConfig,
    _StringView,
    _TargetView,
    _TypeRefView,
    copy_event,
    encoded,
    library,
    view,
)
from .models import Error, Event, Family, MessageHandle, Peer, Profile, Qos, RuntimeConfig, Target, TypeRef


class Runtime:
    def __init__(self, config: RuntimeConfig) -> None:
        self._lib = library()
        self._configure_symbols()
        self._runtime = self._lib.yunlink_v2_runtime_create()
        if not self._runtime:
            raise Error(13)
        self.events: queue.SimpleQueue[Event] = queue.SimpleQueue()
        self._callback = _Callback(self._on_event)
        self._start(config)
        self._token = self._lib.yunlink_v2_runtime_subscribe(self._runtime, self._callback, None)
        if not self._token:
            self.close()
            raise Error(13)

    def _configure_symbols(self) -> None:
        lib = self._lib
        lib.yunlink_v2_runtime_create.restype = ctypes.c_void_p
        lib.yunlink_v2_runtime_destroy.argtypes = [ctypes.c_void_p]
        lib.yunlink_v2_runtime_destroy.restype = None
        lib.yunlink_v2_runtime_stop.argtypes = [ctypes.c_void_p]
        lib.yunlink_v2_runtime_stop.restype = None
        lib.yunlink_v2_runtime_listening_port.argtypes = [ctypes.c_void_p]
        lib.yunlink_v2_runtime_listening_port.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_set_entity_uids.argtypes = [
            ctypes.c_void_p, ctypes.POINTER(_StringView), ctypes.c_size_t,
        ]
        lib.yunlink_v2_runtime_set_entity_uids.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_start.argtypes = [ctypes.c_void_p, ctypes.POINTER(_RuntimeConfig)]
        lib.yunlink_v2_runtime_start.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_connect.argtypes = [ctypes.c_void_p, _StringView, ctypes.c_uint16, ctypes.POINTER(_Peer)]
        lib.yunlink_v2_runtime_connect.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_open_session.argtypes = [ctypes.c_void_p, _StringView]
        lib.yunlink_v2_runtime_open_session.restype = ctypes.c_uint64
        lib.yunlink_v2_runtime_session_endpoint_uid.argtypes = [ctypes.c_void_p, _StringView, ctypes.c_uint64, ctypes.c_void_p, ctypes.c_size_t]
        lib.yunlink_v2_runtime_session_endpoint_uid.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_close_peer.argtypes = [ctypes.c_void_p, _StringView]
        lib.yunlink_v2_runtime_close_peer.restype = None
        lib.yunlink_v2_runtime_session_has_profile.argtypes = [ctypes.c_void_p, _StringView, ctypes.c_uint64, _StringView, ctypes.c_uint16]
        lib.yunlink_v2_runtime_session_has_profile.restype = ctypes.c_uint8
        lib.yunlink_v2_runtime_session_supports_profile.argtypes = [
            ctypes.c_void_p, _StringView, ctypes.c_uint64, _StringView,
            ctypes.c_uint16, ctypes.c_uint16,
        ]
        lib.yunlink_v2_runtime_session_supports_profile.restype = ctypes.c_uint8
        lib.yunlink_v2_runtime_subscribe.argtypes = [ctypes.c_void_p, _Callback, ctypes.c_void_p]
        lib.yunlink_v2_runtime_subscribe.restype = ctypes.c_uint64
        lib.yunlink_v2_runtime_unsubscribe.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        lib.yunlink_v2_runtime_unsubscribe.restype = None
        lib.yunlink_v2_runtime_publish.argtypes = [
            ctypes.c_void_p, _StringView, ctypes.c_uint64, ctypes.c_uint8, ctypes.c_uint8,
            _TargetView, _TypeRefView, _BytesView, ctypes.c_uint64, ctypes.c_uint32,
            ctypes.c_uint8, _StringView, ctypes.POINTER(_Handle),
        ]
        lib.yunlink_v2_runtime_publish.restype = ctypes.c_uint16

    def _start(self, config: RuntimeConfig) -> None:
        strings = [encoded(config.endpoint_uid), encoded(config.display_name), encoded(config.shared_secret)]
        profile_strings: list[bytes] = []

        def profiles(values: Sequence[Profile]) -> ctypes.Array[_ProfileView]:
            views: list[_ProfileView] = []
            for item in values:
                profile_strings.extend((encoded(item.profile_id), encoded(item.schema_digest)))
                views.append(_ProfileView(view(profile_strings[-2]), item.major, item.minor, view(profile_strings[-1])))
            return (_ProfileView * len(views))(*views)

        offered, required = profiles(config.profiles), profiles(config.required_profiles)
        native = _RuntimeConfig(
            ctypes.sizeof(_RuntimeConfig), view(strings[0]), view(strings[1]), view(strings[2]),
            config.tcp_listen_port, offered if len(offered) else ctypes.POINTER(_ProfileView)(), len(offered),
            required if len(required) else ctypes.POINTER(_ProfileView)(), len(required),
        )
        code = self._lib.yunlink_v2_runtime_start(self._runtime, ctypes.byref(native))
        if code:
            self._lib.yunlink_v2_runtime_destroy(self._runtime)
            self._runtime = None
            raise Error(code)

    def _on_event(self, event: ctypes.POINTER(_Event), _user_data: int) -> None:
        if event:
            self.events.put(copy_event(event.contents))

    def connect(self, ip: str, port: int) -> Peer:
        native = _Peer()
        code = self._lib.yunlink_v2_runtime_connect(self._runtime, view(encoded(ip)), port, ctypes.byref(native))
        if code:
            raise Error(code)
        return Peer(native.id.split(b"\0", 1)[0].decode(), native.ip.split(b"\0", 1)[0].decode(), native.port)

    @property
    def listening_port(self) -> int:
        return int(self._lib.yunlink_v2_runtime_listening_port(self._runtime))

    def set_entity_uids(self, entity_uids: Sequence[str]) -> None:
        values = [encoded(uid) for uid in entity_uids]
        views = (_StringView * len(values))(*(view(uid) for uid in values))
        code = self._lib.yunlink_v2_runtime_set_entity_uids(
            self._runtime, views if values else ctypes.POINTER(_StringView)(), len(values)
        )
        if code:
            raise Error(code)

    def open_session(self, peer: Peer) -> int:
        session_id = self._lib.yunlink_v2_runtime_open_session(self._runtime, view(encoded(peer.peer_id)))
        if not session_id:
            raise Error(8)
        return session_id

    def close_peer(self, peer: Peer) -> None:
        self._lib.yunlink_v2_runtime_close_peer(self._runtime, view(encoded(peer.peer_id)))

    def session_endpoint_uid(self, peer: Peer, session_id: int) -> str:
        uid = ctypes.create_string_buffer(129)
        code = self._lib.yunlink_v2_runtime_session_endpoint_uid(self._runtime, view(encoded(peer.peer_id)), session_id, uid, len(uid))
        if code:
            raise Error(code)
        return uid.value.decode("ascii")

    def session_has_profile(self, peer: Peer, session_id: int, profile_id: str, major: int) -> bool:
        return bool(self._lib.yunlink_v2_runtime_session_has_profile(
            self._runtime, view(encoded(peer.peer_id)), session_id, view(encoded(profile_id)), major
        ))

    def session_supports_profile(
        self, peer: Peer, session_id: int, profile_id: str, major: int, minimum_minor: int
    ) -> bool:
        return bool(self._lib.yunlink_v2_runtime_session_supports_profile(
            self._runtime, view(encoded(peer.peer_id)), session_id,
            view(encoded(profile_id)), major, minimum_minor
        ))

    def configuration(self, timeout_s: float = 5.0):
        """Return the generic Configuration Service client for this runtime."""
        from ..configuration_client import ConfigurationClient

        return ConfigurationClient(self, timeout_s)

    def publish(
        self, peer: Peer, session_id: int, family: Family, operation: int, target: Target,
        type_ref: TypeRef, payload: bytes, *, correlation_id: int = 0, ttl_ms: int = 0,
        qos: Qos = Qos.RELIABLE_ORDERED, source_entity_uid: str = "",
    ) -> MessageHandle:
        strings = [encoded(peer.peer_id), encoded(type_ref.profile_id), encoded(type_ref.type_name)]
        uid_bytes = [encoded(uid) for uid in target.uids]
        uid_views = (_StringView * len(uid_bytes))(*(view(uid) for uid in uid_bytes))
        payload_buffer = (ctypes.c_uint8 * len(payload)).from_buffer_copy(payload)
        handle = _Handle()
        code = self._lib.yunlink_v2_runtime_publish(
            self._runtime, view(strings[0]), session_id, family, operation,
            _TargetView(target.scope, uid_views, len(uid_views)),
            _TypeRefView(view(strings[1]), type_ref.major, type_ref.minor, view(strings[2])),
            _BytesView(payload_buffer, len(payload)), correlation_id, ttl_ms, qos,
            view(encoded(source_entity_uid)), ctypes.byref(handle),
        )
        if code:
            raise Error(code)
        return MessageHandle(handle.session_id, handle.message_id, handle.correlation_id)

    def close(self) -> None:
        if not getattr(self, "_runtime", None):
            return
        if getattr(self, "_token", 0):
            self._lib.yunlink_v2_runtime_unsubscribe(self._runtime, self._token)
            self._token = 0
        self._lib.yunlink_v2_runtime_stop(self._runtime)
        self._lib.yunlink_v2_runtime_destroy(self._runtime)
        self._runtime = None

    def __enter__(self) -> "Runtime":
        return self

    def __exit__(self, *_args: object) -> None:
        self.close()

    def __del__(self) -> None:
        self.close()
