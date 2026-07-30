"""Private ctypes declarations and callback copying for ABI 2."""

from __future__ import annotations

import ctypes
import pathlib

from .models import Event, Target, TargetScope, TypeRef


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
    _fields_ = [("id", ctypes.c_char * 256), ("ip", ctypes.c_char * 64), ("port", ctypes.c_uint16)]


class _TargetView(ctypes.Structure):
    _fields_ = [("scope", ctypes.c_uint8), ("uids", ctypes.POINTER(_StringView)), ("uid_count", ctypes.c_size_t)]


class _TypeRefView(ctypes.Structure):
    _fields_ = [
        ("profile_id", _StringView),
        ("major", ctypes.c_uint16),
        ("minor", ctypes.c_uint16),
        ("type_name", _StringView),
    ]


class _Handle(ctypes.Structure):
    _fields_ = [("session_id", ctypes.c_uint64), ("message_id", ctypes.c_uint64), ("correlation_id", ctypes.c_uint64)]


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


def library() -> ctypes.CDLL:
    root = pathlib.Path(__file__).resolve().parent.parent
    patterns = ("libyunlink_ffi*.dylib", "libyunlink_ffi*.so", "yunlink_ffi*.dll")
    for pattern in patterns:
        for candidate in root.glob(pattern):
            return ctypes.CDLL(str(candidate))
    raise RuntimeError(f"YunLink ABI 2 library is not installed beside {root}")


def encoded(value: str) -> bytes:
    return value.encode("utf-8")


def view(value: bytes) -> _StringView:
    return _StringView(ctypes.cast(ctypes.c_char_p(value), ctypes.c_void_p), len(value))


def copy_event(value: _Event) -> Event:
    def text(item: _StringView) -> str:
        return ctypes.string_at(item.data, item.len).decode("utf-8", errors="replace") if item.data and item.len else ""

    uids = tuple(text(value.target.uids[index]) for index in range(value.target.uid_count))
    payload = bytes(value.payload.data[: value.payload.len]) if value.payload.len else b""
    return Event(
        kind=value.kind,
        peer_id=text(value.peer_id),
        link_up=bool(value.link_up),
        error_code=value.error_code,
        message=text(value.message),
        session_state=value.session_state,
        authenticated=bool(value.session_authenticated),
        session_id=value.session_id,
        family=value.family,
        operation=value.operation,
        qos=value.qos_class,
        message_id=value.message_id,
        correlation_id=value.correlation_id,
        source_endpoint_uid=text(value.source_endpoint_uid),
        source_entity_uid=text(value.source_entity_uid),
        target=Target(TargetScope(value.target.scope or TargetScope.BROADCAST), uids),
        type_ref=TypeRef(text(value.type_ref.profile_id), value.type_ref.major, text(value.type_ref.type_name), value.type_ref.minor),
        payload=payload,
    )
