"""Public value types for deterministic YunLink Core payloads."""

from __future__ import annotations

import dataclasses
import enum

from .v2 import TypeRef


class Availability(enum.IntEnum):
    UNKNOWN = 0
    ONLINE = 1
    DEGRADED = 2
    OFFLINE = 3


class ActionPhase(enum.IntEnum):
    RECEIVED = 1
    ACCEPTED = 2
    RUNNING = 3
    SUCCEEDED = 4
    FAILED = 5
    CANCELLED = 6
    EXPIRED = 7

    @property
    def terminal(self) -> bool:
        return self >= self.SUCCEEDED


@dataclasses.dataclass(frozen=True)
class EntityDescriptor:
    entity_uid: str
    kind: str
    display_name: str = ""
    hardware_id: str = ""
    attributes: dict[str, str] = dataclasses.field(default_factory=dict)
    capabilities: tuple[str, ...] = ()
    availability: Availability = Availability.UNKNOWN


@dataclasses.dataclass(frozen=True)
class EntityDirectory:
    endpoint_uid: str
    revision: str
    entities: tuple[EntityDescriptor, ...]


@dataclasses.dataclass(frozen=True)
class AttachmentRequest:
    expected_revision: str
    entity_uids: tuple[str, ...]


@dataclasses.dataclass(frozen=True)
class AttachmentResponse:
    success: bool
    revision: str
    attached_entity_uids: tuple[str, ...]
    message: str = ""


@dataclasses.dataclass(frozen=True)
class AuthorityRequest:
    authority_scope: str
    lease_ttl_ms: int = 5000
    allow_preempt: bool = False


@dataclasses.dataclass(frozen=True)
class AuthorityStatus:
    authority_scope: str
    state: str
    lease_ttl_ms: int
    reason_code: int
    detail: str = ""


@dataclasses.dataclass(frozen=True)
class StreamDescriptor:
    stream_uid: str
    type_ref: TypeRef
    encoding: str
    metadata: dict[str, str] = dataclasses.field(default_factory=dict)


@dataclasses.dataclass(frozen=True)
class StreamCatalog:
    revision: str
    streams: tuple[StreamDescriptor, ...]


@dataclasses.dataclass(frozen=True)
class StreamSubscription:
    stream_uid: str
    max_rate_hz: float = 10.0
    max_payload_bytes: int = 65536


@dataclasses.dataclass(frozen=True)
class StreamSubscriptionStatus:
    success: bool
    subscribed: bool
    stream_uid: str
    max_rate_hz: float
    max_payload_bytes: int
    message: str = ""


@dataclasses.dataclass(frozen=True)
class StreamSample:
    stream_uid: str
    encoding: str
    metadata: dict[str, str]
    source_timestamp_ns: int
    sequence: int
    data: bytes


@dataclasses.dataclass(frozen=True)
class ActionUpdate:
    phase: ActionPhase
    result_code: int = 0
    progress_percent: int = 0
    detail: str = ""
