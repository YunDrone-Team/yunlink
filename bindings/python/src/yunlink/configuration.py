from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import Any


class ConfigValueType(IntEnum):
    BOOL = 1
    INT64 = 2
    DOUBLE = 3
    STRING = 4
    STRING_LIST = 5


class ConfigStatus(IntEnum):
    OK = 0
    NOT_FOUND = 1
    UNSUPPORTED = 2
    UNAUTHENTICATED = 3
    UNAUTHORIZED = 4
    CONFLICT = 5
    INVALID = 6
    UNSAFE_STATE = 7
    INTERNAL_ERROR = 8


class ConfigApplyRequirement(IntEnum):
    NONE = 0
    COMPONENT_RESTART = 1
    ENDPOINT_RESTART = 2
    DEVICE_REBOOT = 3
    MANUAL = 4


class ConfigApplyOutcome(IntEnum):
    APPLIED = 1
    RESTART_SCHEDULED = 2
    MANUAL_ACTION_REQUIRED = 3
    FAILED = 4


@dataclass(frozen=True)
class ConfigValue:
    type: ConfigValueType
    value: bool | int | float | str | tuple[str, ...]

    @staticmethod
    def string_list(values: list[str] | tuple[str, ...]) -> "ConfigValue":
        return ConfigValue(ConfigValueType.STRING_LIST, tuple(values))

    def to_native(self) -> dict[str, Any]:
        value: Any = list(self.value) if self.type == ConfigValueType.STRING_LIST else self.value
        return {"type": int(self.type), "value": value}


@dataclass(frozen=True)
class ConfigResourceDescriptor:
    id: str
    title: str
    description: str
    readable: bool
    writable: bool
    apply_supported: bool


@dataclass(frozen=True)
class ConfigChoice:
    value: ConfigValue
    label: str


@dataclass(frozen=True)
class ConfigFieldSchema:
    path: str
    title: str
    description: str
    type: ConfigValueType
    required: bool
    read_only: bool
    sensitive: bool
    minimum: float | None
    maximum: float | None
    validation_pattern: str
    choices: tuple[ConfigChoice, ...]


@dataclass(frozen=True)
class ConfigFieldValue:
    path: str
    value: ConfigValue

    def to_native(self) -> dict[str, Any]:
        return {"path": self.path, "value": self.value.to_native()}


@dataclass(frozen=True)
class ConfigSnapshot:
    resource_id: str
    revision: str
    applied_revision: str
    values: tuple[ConfigFieldValue, ...]


@dataclass(frozen=True)
class ConfigFieldError:
    path: str
    code: str
    message: str


@dataclass(frozen=True)
class ConfigEffects:
    requirement: ConfigApplyRequirement
    affected_components: tuple[str, ...]
    reconnect_expected: bool


@dataclass(frozen=True)
class ConfigurationHandle:
    message_id: int
    session_id: int
    created_at_ms: int
    ttl_ms: int


@dataclass(frozen=True)
class ConfigResponseBase:
    session_id: int
    message_id: int
    correlation_id: int
    status: ConfigStatus
    message: str


@dataclass(frozen=True)
class ConfigResourceListResponse(ConfigResponseBase):
    resources: tuple[ConfigResourceDescriptor, ...]


@dataclass(frozen=True)
class ConfigResourceDescribeResponse(ConfigResponseBase):
    resource: ConfigResourceDescriptor
    fields: tuple[ConfigFieldSchema, ...]


@dataclass(frozen=True)
class ConfigResourceGetResponse(ConfigResponseBase):
    snapshot: ConfigSnapshot


@dataclass(frozen=True)
class ConfigResourcePatchResponse(ConfigResponseBase):
    snapshot: ConfigSnapshot
    errors: tuple[ConfigFieldError, ...]
    effects: ConfigEffects


@dataclass(frozen=True)
class ConfigResourceApplyResponse(ConfigResponseBase):
    applied_revision: str
    outcome: ConfigApplyOutcome
    effects: ConfigEffects


def _value(data: dict[str, Any]) -> ConfigValue:
    value_type = ConfigValueType(data["type"])
    value = data["value"]
    if value_type == ConfigValueType.STRING_LIST:
        value = tuple(value)
    return ConfigValue(value_type, value)


def _descriptor(data: dict[str, Any]) -> ConfigResourceDescriptor:
    return ConfigResourceDescriptor(**data)


def _snapshot(data: dict[str, Any]) -> ConfigSnapshot:
    return ConfigSnapshot(
        resource_id=data["resource_id"],
        revision=data["revision"],
        applied_revision=data["applied_revision"],
        values=tuple(ConfigFieldValue(item["path"], _value(item["value"])) for item in data["values"]),
    )


def _effects(data: dict[str, Any]) -> ConfigEffects:
    return ConfigEffects(
        requirement=ConfigApplyRequirement(data["requirement"]),
        affected_components=tuple(data["affected_components"]),
        reconnect_expected=data["reconnect_expected"],
    )


def coerce_configuration_response(data: dict[str, Any]) -> ConfigResponseBase | None:
    common = {
        "session_id": data["session_id"],
        "message_id": data["message_id"],
        "correlation_id": data["correlation_id"],
        "status": ConfigStatus(data["status"]),
        "message": data["message"],
    }
    kind = data.get("type")
    if kind == "configuration_list":
        return ConfigResourceListResponse(
            **common, resources=tuple(_descriptor(item) for item in data["resources"])
        )
    if kind == "configuration_describe":
        fields = tuple(
            ConfigFieldSchema(
                path=item["path"],
                title=item["title"],
                description=item["description"],
                type=ConfigValueType(item["type"]),
                required=item["required"],
                read_only=item["read_only"],
                sensitive=item["sensitive"],
                minimum=item["minimum"],
                maximum=item["maximum"],
                validation_pattern=item["validation_pattern"],
                choices=tuple(
                    ConfigChoice(_value(choice["value"]), choice["label"])
                    for choice in item["choices"]
                ),
            )
            for item in data["fields"]
        )
        return ConfigResourceDescribeResponse(
            **common, resource=_descriptor(data["resource"]), fields=fields
        )
    if kind == "configuration_get":
        return ConfigResourceGetResponse(**common, snapshot=_snapshot(data["snapshot"]))
    if kind == "configuration_patch":
        return ConfigResourcePatchResponse(
            **common,
            snapshot=_snapshot(data["snapshot"]),
            errors=tuple(ConfigFieldError(**item) for item in data["errors"]),
            effects=_effects(data["effects"]),
        )
    if kind == "configuration_apply":
        return ConfigResourceApplyResponse(
            **common,
            applied_revision=data["applied_revision"],
            outcome=ConfigApplyOutcome(data["outcome"]),
            effects=_effects(data["effects"]),
        )
    return None
