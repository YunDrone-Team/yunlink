"""Authenticated UDP discovery backed by the YunLink native codecs."""

from __future__ import annotations

import ctypes
import dataclasses
import secrets
import socket
import time

from .v2.ffi import (
    _BytesView,
    _DiscoveryEntityView,
    _KeyValueView,
    _ProfileView,
    _StringView,
    copy_text,
    encoded,
    library,
    view,
)
from .v2.models import Error, Profile


@dataclasses.dataclass(frozen=True)
class DiscoveredEntity:
    entity_uid: str
    kind: str
    display_name: str
    availability: int
    agent_id: int
    attributes: dict[str, str]


@dataclasses.dataclass(frozen=True)
class Advertisement:
    endpoint_uid: str
    display_name: str
    ip: str
    tcp_port: int
    capabilities: tuple[str, ...]
    profiles: tuple[Profile, ...]
    attributes: dict[str, str]
    entities: tuple[DiscoveredEntity, ...]
    started_at_ms: int
    sequence: int


def _configure(lib: ctypes.CDLL) -> None:
    lib.yunlink_v2_discovery_encode_query.argtypes = [
        ctypes.c_uint64, ctypes.c_uint16, ctypes.c_uint8, _StringView,
        ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t),
    ]
    lib.yunlink_v2_discovery_encode_query.restype = ctypes.c_uint16
    lib.yunlink_v2_discovery_decode_reply.argtypes = [_BytesView, _StringView, ctypes.c_uint64]
    lib.yunlink_v2_discovery_decode_reply.restype = ctypes.c_void_p
    lib.yunlink_v2_discovery_advertisement_destroy.argtypes = [ctypes.c_void_p]
    lib.yunlink_v2_discovery_endpoint_uid.argtypes = [ctypes.c_void_p]
    lib.yunlink_v2_discovery_endpoint_uid.restype = _StringView
    lib.yunlink_v2_discovery_display_name.argtypes = [ctypes.c_void_p]
    lib.yunlink_v2_discovery_display_name.restype = _StringView
    for name in ("tcp_port", "capability_count", "profile_count", "attribute_count", "entity_count"):
        function = getattr(lib, f"yunlink_v2_discovery_{name}")
        function.argtypes = [ctypes.c_void_p]
    lib.yunlink_v2_discovery_tcp_port.restype = ctypes.c_uint16
    for name in ("capability_count", "profile_count", "attribute_count", "entity_count"):
        getattr(lib, f"yunlink_v2_discovery_{name}").restype = ctypes.c_size_t
    for name in ("started_at_ms", "sequence"):
        function = getattr(lib, f"yunlink_v2_discovery_{name}")
        function.argtypes = [ctypes.c_void_p]
        function.restype = ctypes.c_uint64
    lib.yunlink_v2_discovery_capability_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    lib.yunlink_v2_discovery_capability_at.restype = _StringView
    lib.yunlink_v2_discovery_profile_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(_ProfileView)]
    lib.yunlink_v2_discovery_profile_at.restype = ctypes.c_uint8
    lib.yunlink_v2_discovery_attribute_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(_KeyValueView)]
    lib.yunlink_v2_discovery_attribute_at.restype = ctypes.c_uint8
    lib.yunlink_v2_discovery_entity_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.POINTER(_DiscoveryEntityView)]
    lib.yunlink_v2_discovery_entity_at.restype = ctypes.c_uint8
    lib.yunlink_v2_discovery_entity_attribute_count.argtypes = [ctypes.c_void_p, ctypes.c_size_t]
    lib.yunlink_v2_discovery_entity_attribute_count.restype = ctypes.c_size_t
    lib.yunlink_v2_discovery_entity_attribute_at.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_size_t, ctypes.POINTER(_KeyValueView)]
    lib.yunlink_v2_discovery_entity_attribute_at.restype = ctypes.c_uint8


def _query(lib: ctypes.CDLL, nonce: int, window_ms: int, secret: bytes) -> bytes:
    output = (ctypes.c_uint8 * 64)()
    size = ctypes.c_size_t()
    code = lib.yunlink_v2_discovery_encode_query(
        nonce, window_ms, 4, view(secret), output, len(output), ctypes.byref(size)
    )
    if code:
        raise Error(code)
    return bytes(output[: size.value])


def _attributes(lib: ctypes.CDLL, handle: int, count: int, entity_index: int | None = None) -> dict[str, str]:
    result: dict[str, str] = {}
    for index in range(count):
        item = _KeyValueView()
        ok = (lib.yunlink_v2_discovery_attribute_at(handle, index, ctypes.byref(item)) if entity_index is None
              else lib.yunlink_v2_discovery_entity_attribute_at(handle, entity_index, index, ctypes.byref(item)))
        if ok:
            result[copy_text(item.key)] = copy_text(item.value)
    return result


def _decode(lib: ctypes.CDLL, payload: bytes, secret: bytes, nonce: int, ip: str) -> Advertisement | None:
    buffer = (ctypes.c_uint8 * len(payload)).from_buffer_copy(payload)
    handle = lib.yunlink_v2_discovery_decode_reply(_BytesView(buffer, len(payload)), view(secret), nonce)
    if not handle:
        return None
    try:
        profiles = []
        for index in range(lib.yunlink_v2_discovery_profile_count(handle)):
            item = _ProfileView()
            if lib.yunlink_v2_discovery_profile_at(handle, index, ctypes.byref(item)):
                profiles.append(Profile(copy_text(item.profile_id), item.major, item.minor, copy_text(item.schema_digest)))
        entities = []
        for index in range(lib.yunlink_v2_discovery_entity_count(handle)):
            item = _DiscoveryEntityView()
            if not lib.yunlink_v2_discovery_entity_at(handle, index, ctypes.byref(item)):
                continue
            count = lib.yunlink_v2_discovery_entity_attribute_count(handle, index)
            entities.append(DiscoveredEntity(copy_text(item.entity_uid), copy_text(item.kind), copy_text(item.display_name),
                                               item.availability, item.agent_id, _attributes(lib, handle, count, index)))
        capabilities = tuple(copy_text(lib.yunlink_v2_discovery_capability_at(handle, index))
                             for index in range(lib.yunlink_v2_discovery_capability_count(handle)))
        return Advertisement(
            copy_text(lib.yunlink_v2_discovery_endpoint_uid(handle)),
            copy_text(lib.yunlink_v2_discovery_display_name(handle)), ip,
            lib.yunlink_v2_discovery_tcp_port(handle), capabilities, tuple(profiles),
            _attributes(lib, handle, lib.yunlink_v2_discovery_attribute_count(handle)),
            tuple(entities), lib.yunlink_v2_discovery_started_at_ms(handle),
            lib.yunlink_v2_discovery_sequence(handle),
        )
    finally:
        lib.yunlink_v2_discovery_advertisement_destroy(handle)


def discover(*, host: str = "255.255.255.255", port: int = 9697, timeout: float = 1.0,
             shared_secret: str = "yunlink-default-secret") -> list[Advertisement]:
    """Return authenticated Bridge advertisements received before ``timeout``."""
    lib = library()
    _configure(lib)
    nonce = secrets.randbits(64) or 1
    window_ms = max(1, min(65535, int(timeout * 1000)))
    secret = encoded(shared_secret)
    request = _query(lib, nonce, window_ms, secret)
    family = socket.AF_INET6 if ":" in host else socket.AF_INET
    found: dict[str, Advertisement] = {}
    with socket.socket(family, socket.SOCK_DGRAM) as udp:
        udp.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
        udp.settimeout(max(0.01, timeout))
        udp.sendto(request, (host, port))
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                break
            udp.settimeout(remaining)
            try:
                payload, sender = udp.recvfrom(8192)
            except socket.timeout:
                break
            advertisement = _decode(lib, payload, secret, nonce, sender[0])
            if advertisement is not None:
                found[advertisement.endpoint_uid] = advertisement
    return list(found.values())
