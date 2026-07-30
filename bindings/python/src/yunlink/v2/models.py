"""Public value types for the YunLink Wire v2 Python facade."""

from __future__ import annotations

import dataclasses
import enum


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
