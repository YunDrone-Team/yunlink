from __future__ import annotations

from dataclasses import dataclass
from enum import Enum, IntEnum
from typing import Any

from .types import EndpointIdentity


class ManagedEntityAvailability(IntEnum):
    UNKNOWN = 0
    ONLINE = 1
    DEGRADED = 2
    OFFLINE = 3


class ManagedEntityAttachmentAction(str, Enum):
    ATTACH = "attach"
    DETACH = "detach"


@dataclass(frozen=True)
class ManagedEntityDescriptor:
    entity_uid: str
    identity: EndpointIdentity
    display_name: str
    hardware_id: str
    capabilities: tuple[str, ...]
    availability: int


@dataclass(frozen=True)
class ManagedEntityDirectory:
    session_id: int
    message_id: int
    correlation_id: int
    success: bool
    message: str
    endpoint_uid: str
    revision: str
    primary_identity: EndpointIdentity
    entities: tuple[ManagedEntityDescriptor, ...]


@dataclass(frozen=True)
class ManagedEntityDirectoryChanged:
    session_id: int
    message_id: int
    correlation_id: int
    endpoint_uid: str
    revision: str


@dataclass(frozen=True)
class ManagedEntityAttachmentResult:
    entity_uid: str
    accepted: bool
    message: str


@dataclass(frozen=True)
class ManagedEntityAttachmentResponse:
    session_id: int
    message_id: int
    correlation_id: int
    success: bool
    message: str
    endpoint_uid: str
    directory_revision: str
    results: tuple[ManagedEntityAttachmentResult, ...]
    attached_entity_uids: tuple[str, ...]


def _identity(data: dict[str, Any]) -> EndpointIdentity:
    return EndpointIdentity(
        agent_type=int(data["agent_type"]),
        agent_id=int(data["agent_id"]),
        role=int(data["role"]),
        group_ids=tuple(int(value) for value in data.get("group_ids", ())),
    )


def coerce_managed_entity_event(data: dict[str, Any]) -> object | None:
    if data.get("type") == "managed_entity_directory_changed":
        return ManagedEntityDirectoryChanged(
            session_id=data["session_id"],
            message_id=data["message_id"],
            correlation_id=data["correlation_id"],
            endpoint_uid=data["endpoint_uid"],
            revision=data["revision"],
        )
    if data.get("type") == "managed_entity_attachment":
        return ManagedEntityAttachmentResponse(
            session_id=data["session_id"],
            message_id=data["message_id"],
            correlation_id=data["correlation_id"],
            success=bool(data["success"]),
            message=data["message"],
            endpoint_uid=data["endpoint_uid"],
            directory_revision=data["directory_revision"],
            results=tuple(
                ManagedEntityAttachmentResult(
                    entity_uid=item["entity_uid"],
                    accepted=bool(item["accepted"]),
                    message=item["message"],
                )
                for item in data.get("results", ())
            ),
            attached_entity_uids=tuple(data.get("attached_entity_uids", ())),
        )
    if data.get("type") != "managed_entity_directory":
        return None
    entities = tuple(
        ManagedEntityDescriptor(
            entity_uid=item["entity_uid"],
            identity=_identity(item["identity"]),
            display_name=item["display_name"],
            hardware_id=item["hardware_id"],
            capabilities=tuple(item.get("capabilities", ())),
            availability=int(item["availability"]),
        )
        for item in data.get("entities", ())
    )
    return ManagedEntityDirectory(
        session_id=data["session_id"],
        message_id=data["message_id"],
        correlation_id=data["correlation_id"],
        success=bool(data["success"]),
        message=data["message"],
        endpoint_uid=data["endpoint_uid"],
        revision=data["revision"],
        primary_identity=_identity(data["primary_identity"]),
        entities=entities,
    )
