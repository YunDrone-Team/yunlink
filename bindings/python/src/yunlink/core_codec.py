"""Deterministic codec for the small set of public YunLink Core messages."""

from __future__ import annotations

import struct
from typing import Callable, TypeVar

from .core import (
    ActionPhase, ActionUpdate, AttachmentRequest, AttachmentResponse, AuthorityRequest,
    AuthorityStatus, Availability, EntityDescriptor, EntityDirectory, StreamCatalog,
    StreamDescriptor, StreamSample, StreamSubscription, StreamSubscriptionStatus,
)
from .v2 import TypeRef

T = TypeVar("T")


class CoreCodecError(ValueError):
    pass


class _Writer:
    def __init__(self) -> None:
        self.data = bytearray()

    def u8(self, value: int) -> None: self.data += struct.pack("<B", value)
    def u16(self, value: int) -> None: self.data += struct.pack("<H", value)
    def u32(self, value: int) -> None: self.data += struct.pack("<I", value)
    def u64(self, value: int) -> None: self.data += struct.pack("<Q", value)
    def f32(self, value: float) -> None: self.data += struct.pack("<f", value)

    def text(self, value: str) -> None:
        encoded = value.encode("utf-8")
        if len(encoded) > 65535: raise CoreCodecError("text exceeds 65535 bytes")
        self.u16(len(encoded)); self.data += encoded

    def blob(self, value: bytes) -> None:
        if len(value) > 0xFFFFFFFF: raise CoreCodecError("payload is too large")
        self.u32(len(value)); self.data += value

    def items(self, values, write: Callable[[object], None]) -> None:
        if len(values) > 65535: raise CoreCodecError("list exceeds 65535 items")
        self.u16(len(values))
        for item in values: write(item)

    def mapping(self, value: dict[str, str]) -> None:
        self.items(sorted(value.items()), lambda item: (self.text(item[0]), self.text(item[1])))


class _Reader:
    def __init__(self, data: bytes) -> None: self.data, self.offset = data, 0

    def take(self, size: int) -> bytes:
        end = self.offset + size
        if end > len(self.data): raise CoreCodecError("truncated core payload")
        value, self.offset = self.data[self.offset:end], end
        return value

    def u8(self) -> int: return struct.unpack("<B", self.take(1))[0]
    def u16(self) -> int: return struct.unpack("<H", self.take(2))[0]
    def u32(self) -> int: return struct.unpack("<I", self.take(4))[0]
    def u64(self) -> int: return struct.unpack("<Q", self.take(8))[0]
    def f32(self) -> float: return struct.unpack("<f", self.take(4))[0]
    def text(self) -> str: return self.take(self.u16()).decode("utf-8")
    def blob(self) -> bytes: return self.take(self.u32())
    def items(self, read: Callable[[], T]) -> tuple[T, ...]: return tuple(read() for _ in range(self.u16()))
    def mapping(self) -> dict[str, str]: return dict(self.items(lambda: (self.text(), self.text())))

    def finish(self, value: T) -> T:
        if self.offset != len(self.data): raise CoreCodecError("trailing core payload bytes")
        return value


def _type_write(w: _Writer, value: TypeRef) -> None:
    w.text(value.profile_id); w.u16(value.major); w.u16(value.minor); w.text(value.type_name)


def _type_read(r: _Reader) -> TypeRef:
    profile_id, major, minor, type_name = r.text(), r.u16(), r.u16(), r.text()
    return TypeRef(profile_id, major, type_name, minor)


def _entity_write(w: _Writer, value: EntityDescriptor) -> None:
    w.text(value.entity_uid); w.text(value.kind); w.text(value.display_name); w.text(value.hardware_id)
    w.mapping(value.attributes); w.items(value.capabilities, w.text); w.u8(value.availability)


def _entity_read(r: _Reader) -> EntityDescriptor:
    return EntityDescriptor(r.text(), r.text(), r.text(), r.text(), r.mapping(), r.items(r.text), Availability(r.u8()))


def encode(value: object) -> bytes:
    w = _Writer()
    if isinstance(value, EntityDirectory):
        w.text(value.endpoint_uid); w.text(value.revision); w.items(value.entities, lambda item: _entity_write(w, item))
    elif isinstance(value, AttachmentRequest):
        w.text(value.expected_revision); w.items(value.entity_uids, w.text)
    elif isinstance(value, AttachmentResponse):
        w.u8(value.success); w.text(value.revision); w.items(value.attached_entity_uids, w.text); w.text(value.message)
    elif isinstance(value, AuthorityRequest):
        w.text(value.authority_scope); w.u32(value.lease_ttl_ms); w.u8(value.allow_preempt)
    elif isinstance(value, AuthorityStatus):
        w.text(value.authority_scope); w.text(value.state); w.u32(value.lease_ttl_ms); w.u16(value.reason_code); w.text(value.detail)
    elif isinstance(value, StreamCatalog):
        w.text(value.revision)
        def stream(item: StreamDescriptor) -> None:
            w.text(item.stream_uid); _type_write(w, item.type_ref); w.text(item.encoding); w.mapping(item.metadata)
        w.items(value.streams, stream)
    elif isinstance(value, StreamSubscription):
        w.text(value.stream_uid); w.f32(value.max_rate_hz); w.u32(value.max_payload_bytes)
    elif isinstance(value, StreamSubscriptionStatus):
        w.u8(value.success); w.u8(value.subscribed); w.text(value.stream_uid); w.f32(value.max_rate_hz); w.u32(value.max_payload_bytes); w.text(value.message)
    elif isinstance(value, StreamSample):
        w.text(value.stream_uid); w.text(value.encoding); w.mapping(value.metadata); w.u64(value.source_timestamp_ns); w.u64(value.sequence); w.blob(value.data)
    elif isinstance(value, ActionUpdate):
        w.u8(value.phase); w.u16(value.result_code); w.u8(value.progress_percent); w.text(value.detail)
    else:
        raise CoreCodecError(f"unsupported core message: {type(value).__name__}")
    return bytes(w.data)


def decode_entity_directory(data: bytes) -> EntityDirectory:
    r = _Reader(data); return r.finish(EntityDirectory(r.text(), r.text(), r.items(lambda: _entity_read(r))))


def decode_attachment_request(data: bytes) -> AttachmentRequest:
    r = _Reader(data); return r.finish(AttachmentRequest(r.text(), r.items(r.text)))


def decode_attachment_response(data: bytes) -> AttachmentResponse:
    r = _Reader(data); success = r.u8(); value = AttachmentResponse(bool(success), r.text(), r.items(r.text), r.text())
    if success > 1: raise CoreCodecError("invalid attachment success flag")
    return r.finish(value)


def decode_authority_request(data: bytes) -> AuthorityRequest:
    r = _Reader(data); scope, lease_ttl_ms, allow_preempt = r.text(), r.u32(), r.u8()
    if allow_preempt > 1: raise CoreCodecError("invalid authority preemption flag")
    return r.finish(AuthorityRequest(scope, lease_ttl_ms, bool(allow_preempt)))


def decode_authority_status(data: bytes) -> AuthorityStatus:
    r = _Reader(data); return r.finish(AuthorityStatus(r.text(), r.text(), r.u32(), r.u16(), r.text()))


def decode_stream_catalog(data: bytes) -> StreamCatalog:
    r = _Reader(data)
    def stream() -> StreamDescriptor:
        return StreamDescriptor(r.text(), _type_read(r), r.text(), r.mapping())
    return r.finish(StreamCatalog(r.text(), r.items(stream)))


def decode_stream_subscription(data: bytes) -> StreamSubscription:
    r = _Reader(data); return r.finish(StreamSubscription(r.text(), r.f32(), r.u32()))


def decode_stream_subscription_status(data: bytes) -> StreamSubscriptionStatus:
    r = _Reader(data); success, subscribed = r.u8(), r.u8()
    value = StreamSubscriptionStatus(bool(success), bool(subscribed), r.text(), r.f32(), r.u32(), r.text())
    if success > 1 or subscribed > 1: raise CoreCodecError("invalid subscription status flags")
    return r.finish(value)


def decode_stream_sample(data: bytes) -> StreamSample:
    r = _Reader(data); return r.finish(StreamSample(r.text(), r.text(), r.mapping(), r.u64(), r.u64(), r.blob()))


def decode_action_update(data: bytes) -> ActionUpdate:
    r = _Reader(data); phase, result, progress, detail = ActionPhase(r.u8()), r.u16(), r.u8(), r.text()
    if progress > 100: raise CoreCodecError("action progress exceeds 100")
    return r.finish(ActionUpdate(phase, result, progress, detail))
