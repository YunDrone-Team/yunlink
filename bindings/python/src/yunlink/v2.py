"""Owned Python facade over the generic YunLink ABI 2."""

from __future__ import annotations

import ctypes
import dataclasses
import enum
import pathlib
import queue
from collections.abc import Sequence


class Error(RuntimeError):
    def __init__(self, code: int) -> None:
        self.code = code
        super().__init__(f"YUNLINK_V2_ERROR({code})")


class Family(enum.IntEnum):
    SESSION = 1
    AUTHORITY = 2
    ENTITY_DIRECTORY = 3
    STREAM = 4
    ACTION = 5
    RPC = 6
    CONFIGURATION = 7
    LOG = 8
    BULK = 9


class Qos(enum.IntEnum):
    RELIABLE_ORDERED = 1
    RELIABLE_LATEST = 2
    BEST_EFFORT = 3
    BULK = 4


class TargetScope(enum.IntEnum):
    ENDPOINT = 1
    ENTITY = 2
    GROUP = 3
    BROADCAST = 4


@dataclasses.dataclass(frozen=True)
class Profile:
    profile_id: str
    major: int
    minor: int = 0
    schema_digest: str = ""


@dataclasses.dataclass(frozen=True)
class TypeRef:
    profile_id: str
    major: int
    type_name: str
    minor: int = 0


@dataclasses.dataclass(frozen=True)
class Target:
    scope: TargetScope
    uids: tuple[str, ...] = ()

    @classmethod
    def entity(cls, uid: str) -> "Target":
        return cls(TargetScope.ENTITY, (uid,))

    @classmethod
    def endpoint(cls, uid: str) -> "Target":
        return cls(TargetScope.ENDPOINT, (uid,))

    @classmethod
    def broadcast(cls) -> "Target":
        return cls(TargetScope.BROADCAST)


@dataclasses.dataclass(frozen=True)
class RuntimeConfig:
    endpoint_uid: str
    tcp_listen_port: int
    display_name: str = "yunlink-endpoint"
    shared_secret: str = "yunlink-default-secret"
    profiles: tuple[Profile, ...] = ()
    required_profiles: tuple[Profile, ...] = ()


@dataclasses.dataclass(frozen=True)
class Peer:
    peer_id: str
    ip: str
    port: int


@dataclasses.dataclass(frozen=True)
class MessageHandle:
    session_id: int
    message_id: int
    correlation_id: int


@dataclasses.dataclass(frozen=True)
class Event:
    kind: int
    peer_id: str
    link_up: bool
    error_code: int
    message: str
    session_state: int
    authenticated: bool
    session_id: int
    family: int
    operation: int
    qos: int
    message_id: int
    correlation_id: int
    source_endpoint_uid: str
    source_entity_uid: str
    target: Target
    type_ref: TypeRef
    payload: bytes


class _StringView(ctypes.Structure):
    _fields_ = [("data", ctypes.c_void_p), ("len", ctypes.c_size_t)]


class _BytesView(ctypes.Structure):
    _fields_ = [("data", ctypes.POINTER(ctypes.c_uint8)), ("len", ctypes.c_size_t)]


class _ProfileView(ctypes.Structure):
    _fields_ = [
        ("profile_id", _StringView),
        ("major", ctypes.c_uint16),
        ("minor", ctypes.c_uint16),
        ("schema_digest", _StringView),
    ]


class _RuntimeConfig(ctypes.Structure):
    _fields_ = [
        ("struct_size", ctypes.c_size_t),
        ("endpoint_uid", _StringView),
        ("display_name", _StringView),
        ("shared_secret", _StringView),
        ("tcp_listen_port", ctypes.c_uint16),
        ("profiles", ctypes.POINTER(_ProfileView)),
        ("profile_count", ctypes.c_size_t),
        ("required_profiles", ctypes.POINTER(_ProfileView)),
        ("required_profile_count", ctypes.c_size_t),
    ]


class _Peer(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_char * 256),
        ("ip", ctypes.c_char * 64),
        ("port", ctypes.c_uint16),
    ]


class _TargetView(ctypes.Structure):
    _fields_ = [
        ("scope", ctypes.c_uint8),
        ("uids", ctypes.POINTER(_StringView)),
        ("uid_count", ctypes.c_size_t),
    ]


class _TypeRefView(ctypes.Structure):
    _fields_ = [
        ("profile_id", _StringView),
        ("major", ctypes.c_uint16),
        ("minor", ctypes.c_uint16),
        ("type_name", _StringView),
    ]


class _Handle(ctypes.Structure):
    _fields_ = [
        ("session_id", ctypes.c_uint64),
        ("message_id", ctypes.c_uint64),
        ("correlation_id", ctypes.c_uint64),
    ]


class _Event(ctypes.Structure):
    _fields_ = [
        ("kind", ctypes.c_uint8),
        ("peer_id", _StringView),
        ("link_up", ctypes.c_uint8),
        ("error_code", ctypes.c_uint16),
        ("message", _StringView),
        ("session_state", ctypes.c_uint8),
        ("session_authenticated", ctypes.c_uint8),
        ("session_id", ctypes.c_uint64),
        ("family", ctypes.c_uint8),
        ("operation", ctypes.c_uint8),
        ("qos_class", ctypes.c_uint8),
        ("message_id", ctypes.c_uint64),
        ("correlation_id", ctypes.c_uint64),
        ("created_at_ms", ctypes.c_uint64),
        ("ttl_ms", ctypes.c_uint32),
        ("source_endpoint_uid", _StringView),
        ("source_entity_uid", _StringView),
        ("target", _TargetView),
        ("type_ref", _TypeRefView),
        ("payload", _BytesView),
    ]


_Callback = ctypes.CFUNCTYPE(None, ctypes.POINTER(_Event), ctypes.c_void_p)


def _library() -> ctypes.CDLL:
    root = pathlib.Path(__file__).resolve().parent
    patterns = ("libyunlink_ffi*.dylib", "libyunlink_ffi*.so", "yunlink_ffi*.dll")
    for pattern in patterns:
        for candidate in root.glob(pattern):
            return ctypes.CDLL(str(candidate))
    raise RuntimeError(f"YunLink ABI 2 library is not installed beside {__file__}")


def _encoded(value: str) -> bytes:
    return value.encode("utf-8")


def _view(value: bytes) -> _StringView:
    return _StringView(ctypes.cast(ctypes.c_char_p(value), ctypes.c_void_p), len(value))


def _copy_text(value: _StringView) -> str:
    if not value.data or not value.len:
        return ""
    return ctypes.string_at(value.data, value.len).decode("utf-8", errors="replace")


def _copy_event(value: _Event) -> Event:
    uids = tuple(_copy_text(value.target.uids[index]) for index in range(value.target.uid_count))
    payload = bytes(value.payload.data[: value.payload.len]) if value.payload.len else b""
    return Event(
        kind=value.kind,
        peer_id=_copy_text(value.peer_id),
        link_up=bool(value.link_up),
        error_code=value.error_code,
        message=_copy_text(value.message),
        session_state=value.session_state,
        authenticated=bool(value.session_authenticated),
        session_id=value.session_id,
        family=value.family,
        operation=value.operation,
        qos=value.qos_class,
        message_id=value.message_id,
        correlation_id=value.correlation_id,
        source_endpoint_uid=_copy_text(value.source_endpoint_uid),
        source_entity_uid=_copy_text(value.source_entity_uid),
        target=Target(TargetScope(value.target.scope or TargetScope.BROADCAST), uids),
        type_ref=TypeRef(
            _copy_text(value.type_ref.profile_id),
            value.type_ref.major,
            _copy_text(value.type_ref.type_name),
            value.type_ref.minor,
        ),
        payload=payload,
    )


class Runtime:
    def __init__(self, config: RuntimeConfig) -> None:
        self._lib = _library()
        self._configure_symbols()
        self._runtime = self._lib.yunlink_v2_runtime_create()
        if not self._runtime:
            raise Error(13)
        self.events: queue.SimpleQueue[Event] = queue.SimpleQueue()
        self._callback = _Callback(self._on_event)
        self._start(config)
        self._token = self._lib.yunlink_v2_runtime_subscribe(
            self._runtime, self._callback, None
        )
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
        lib.yunlink_v2_runtime_start.argtypes = [ctypes.c_void_p, ctypes.POINTER(_RuntimeConfig)]
        lib.yunlink_v2_runtime_start.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_connect.argtypes = [
            ctypes.c_void_p,
            _StringView,
            ctypes.c_uint16,
            ctypes.POINTER(_Peer),
        ]
        lib.yunlink_v2_runtime_connect.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_open_session.argtypes = [ctypes.c_void_p, _StringView]
        lib.yunlink_v2_runtime_open_session.restype = ctypes.c_uint64
        lib.yunlink_v2_runtime_session_endpoint_uid.argtypes = [
            ctypes.c_void_p,
            _StringView,
            ctypes.c_uint64,
            ctypes.c_void_p,
            ctypes.c_size_t,
        ]
        lib.yunlink_v2_runtime_session_endpoint_uid.restype = ctypes.c_uint16
        lib.yunlink_v2_runtime_close_peer.argtypes = [ctypes.c_void_p, _StringView]
        lib.yunlink_v2_runtime_close_peer.restype = None
        lib.yunlink_v2_runtime_session_has_profile.argtypes = [
            ctypes.c_void_p,
            _StringView,
            ctypes.c_uint64,
            _StringView,
            ctypes.c_uint16,
        ]
        lib.yunlink_v2_runtime_session_has_profile.restype = ctypes.c_uint8
        lib.yunlink_v2_runtime_subscribe.argtypes = [ctypes.c_void_p, _Callback, ctypes.c_void_p]
        lib.yunlink_v2_runtime_subscribe.restype = ctypes.c_uint64
        lib.yunlink_v2_runtime_unsubscribe.argtypes = [ctypes.c_void_p, ctypes.c_uint64]
        lib.yunlink_v2_runtime_unsubscribe.restype = None
        lib.yunlink_v2_runtime_publish.argtypes = [
            ctypes.c_void_p,
            _StringView,
            ctypes.c_uint64,
            ctypes.c_uint8,
            ctypes.c_uint8,
            _TargetView,
            _TypeRefView,
            _BytesView,
            ctypes.c_uint64,
            ctypes.c_uint32,
            ctypes.c_uint8,
            _StringView,
            ctypes.POINTER(_Handle),
        ]
        lib.yunlink_v2_runtime_publish.restype = ctypes.c_uint16

    def _start(self, config: RuntimeConfig) -> None:
        strings = [
            _encoded(config.endpoint_uid),
            _encoded(config.display_name),
            _encoded(config.shared_secret),
        ]
        profile_strings: list[bytes] = []

        def profiles(values: Sequence[Profile]) -> ctypes.Array[_ProfileView]:
            views: list[_ProfileView] = []
            for item in values:
                profile_strings.extend((_encoded(item.profile_id), _encoded(item.schema_digest)))
                views.append(
                    _ProfileView(
                        _view(profile_strings[-2]), item.major, item.minor, _view(profile_strings[-1])
                    )
                )
            return (_ProfileView * len(views))(*views)

        offered = profiles(config.profiles)
        required = profiles(config.required_profiles)
        offered_view = offered if len(offered) else ctypes.POINTER(_ProfileView)()
        required_view = required if len(required) else ctypes.POINTER(_ProfileView)()
        native = _RuntimeConfig(
            ctypes.sizeof(_RuntimeConfig),
            _view(strings[0]),
            _view(strings[1]),
            _view(strings[2]),
            config.tcp_listen_port,
            offered_view,
            len(offered),
            required_view,
            len(required),
        )
        code = self._lib.yunlink_v2_runtime_start(self._runtime, ctypes.byref(native))
        if code:
            self._lib.yunlink_v2_runtime_destroy(self._runtime)
            self._runtime = None
            raise Error(code)

    def _on_event(self, event: ctypes.POINTER(_Event), _user_data: int) -> None:
        if event:
            self.events.put(_copy_event(event.contents))

    def connect(self, ip: str, port: int) -> Peer:
        encoded = _encoded(ip)
        native = _Peer()
        code = self._lib.yunlink_v2_runtime_connect(
            self._runtime, _view(encoded), port, ctypes.byref(native)
        )
        if code:
            raise Error(code)
        return Peer(native.id.split(b"\0", 1)[0].decode(), native.ip.split(b"\0", 1)[0].decode(), native.port)

    def open_session(self, peer: Peer) -> int:
        peer_id = _encoded(peer.peer_id)
        session_id = self._lib.yunlink_v2_runtime_open_session(self._runtime, _view(peer_id))
        if not session_id:
            raise Error(8)
        return session_id

    def close_peer(self, peer: Peer) -> None:
        peer_id = _encoded(peer.peer_id)
        self._lib.yunlink_v2_runtime_close_peer(self._runtime, _view(peer_id))

    def session_endpoint_uid(self, peer: Peer, session_id: int) -> str:
        peer_id = _encoded(peer.peer_id)
        uid = ctypes.create_string_buffer(129)
        code = self._lib.yunlink_v2_runtime_session_endpoint_uid(
            self._runtime, _view(peer_id), session_id, uid, len(uid)
        )
        if code:
            raise Error(code)
        return uid.value.decode("ascii")

    def session_has_profile(
        self, peer: Peer, session_id: int, profile_id: str, major: int
    ) -> bool:
        peer_id = _encoded(peer.peer_id)
        profile = _encoded(profile_id)
        return bool(
            self._lib.yunlink_v2_runtime_session_has_profile(
                self._runtime, _view(peer_id), session_id, _view(profile), major
            )
        )

    def configuration(self, timeout_s: float = 5.0):
        """Return the generic Configuration Service client for this runtime."""
        from .configuration_client import ConfigurationClient

        return ConfigurationClient(self, timeout_s)

    def publish(
        self,
        peer: Peer,
        session_id: int,
        family: Family,
        operation: int,
        target: Target,
        type_ref: TypeRef,
        payload: bytes,
        *,
        correlation_id: int = 0,
        ttl_ms: int = 0,
        qos: Qos = Qos.RELIABLE_ORDERED,
        source_entity_uid: str = "",
    ) -> MessageHandle:
        strings = [_encoded(peer.peer_id), _encoded(type_ref.profile_id), _encoded(type_ref.type_name)]
        source = _encoded(source_entity_uid)
        uid_bytes = [_encoded(uid) for uid in target.uids]
        uid_views = (_StringView * len(uid_bytes))(*(_view(uid) for uid in uid_bytes))
        native_target = _TargetView(target.scope, uid_views, len(uid_views))
        native_type = _TypeRefView(
            _view(strings[1]), type_ref.major, type_ref.minor, _view(strings[2])
        )
        payload_buffer = (ctypes.c_uint8 * len(payload)).from_buffer_copy(payload)
        native_payload = _BytesView(payload_buffer, len(payload))
        handle = _Handle()
        code = self._lib.yunlink_v2_runtime_publish(
            self._runtime,
            _view(strings[0]),
            session_id,
            family,
            operation,
            native_target,
            native_type,
            native_payload,
            correlation_id,
            ttl_ms,
            qos,
            _view(source),
            ctypes.byref(handle),
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
