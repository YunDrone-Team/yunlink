"""Owned, provider-neutral Configuration Service models for YunLink."""

from __future__ import annotations

import dataclasses
import enum


class ConfigValueType(enum.IntEnum):
    BOOL = 1
    INT64 = 2
    DOUBLE = 3
    STRING = 4
    STRING_LIST = 5
    DOUBLE_LIST = 6


class ConfigServiceStatus(enum.IntEnum):
    OK = 0
    NOT_FOUND = 1
    UNSUPPORTED = 2
    UNAUTHENTICATED = 3
    UNAUTHORIZED = 4
    CONFLICT = 5
    INVALID = 6
    UNSAFE_STATE = 7
    INTERNAL_ERROR = 8


class ConfigApplyRequirement(enum.IntEnum):
    NONE = 0
    COMPONENT_RESTART = 1
    ENDPOINT_RESTART = 2
    DEVICE_REBOOT = 3
    MANUAL = 4


class ConfigApplyOutcome(enum.IntEnum):
    APPLIED = 1
    RESTART_SCHEDULED = 2
    MANUAL_ACTION_REQUIRED = 3
    FAILED = 4


class ConfigFieldUpdatePolicy(enum.IntEnum):
    HOT_RELOAD = 0
    COMPONENT_RESTART = 1
    ENDPOINT_RESTART = 2
    DEVICE_REBOOT = 3
    MANUAL = 4


class ConfigVariantSource(enum.IntEnum):
    DEFAULT = 1
    ACTIVE = 2


@dataclasses.dataclass(frozen=True)
class ConfigValue:
    value_type: ConfigValueType
    value: bool | int | float | str | tuple[str, ...] | tuple[float, ...]

    @classmethod
    def bool(cls, value: bool) -> "ConfigValue":
        return cls(ConfigValueType.BOOL, value)

    @classmethod
    def int64(cls, value: int) -> "ConfigValue":
        return cls(ConfigValueType.INT64, value)

    @classmethod
    def double(cls, value: float) -> "ConfigValue":
        return cls(ConfigValueType.DOUBLE, value)

    @classmethod
    def string(cls, value: str) -> "ConfigValue":
        return cls(ConfigValueType.STRING, value)

    @classmethod
    def string_list(cls, value: tuple[str, ...] | list[str]) -> "ConfigValue":
        return cls(ConfigValueType.STRING_LIST, tuple(value))

    @classmethod
    def double_list(cls, value: tuple[float, ...] | list[float]) -> "ConfigValue":
        return cls(ConfigValueType.DOUBLE_LIST, tuple(value))


@dataclasses.dataclass(frozen=True)
class ConfigResourceDescriptor:
    id: str
    title: str
    description: str
    readable: bool = True
    writable: bool = False
    apply_supported: bool = False
    variants_supported: bool = False


@dataclasses.dataclass(frozen=True)
class ConfigChoice:
    value: ConfigValue
    label: str


@dataclasses.dataclass(frozen=True)
class ConfigFieldSchema:
    path: str
    title: str
    description: str
    value_type: ConfigValueType
    required: bool = False
    read_only: bool = False
    sensitive: bool = False
    minimum: float | None = None
    maximum: float | None = None
    validation_pattern: str = ""
    choices: tuple[ConfigChoice, ...] = ()
    group_path: str = ""
    update_policy: ConfigFieldUpdatePolicy = ConfigFieldUpdatePolicy.MANUAL
    unit: str = ""


@dataclasses.dataclass(frozen=True)
class ConfigFieldValue:
    path: str
    value: ConfigValue


@dataclasses.dataclass(frozen=True)
class ConfigSnapshot:
    resource_id: str
    revision: str
    applied_revision: str
    variant_id: str = ""
    active_variant_id: str = ""
    values: tuple[ConfigFieldValue, ...] = ()


@dataclasses.dataclass(frozen=True)
class ConfigFieldError:
    path: str
    code: str
    message: str


@dataclasses.dataclass(frozen=True)
class ConfigEffects:
    requirement: ConfigApplyRequirement = ConfigApplyRequirement.NONE
    affected_components: tuple[str, ...] = ()
    reconnect_expected: bool = False


@dataclasses.dataclass(frozen=True)
class ConfigVariantDescriptor:
    id: str
    title: str
    revision: str
    modified_at_ns: int
    active: bool
    mutable_variant: bool


@dataclasses.dataclass(frozen=True)
class ConfigResourceListRequest:
    pass


@dataclasses.dataclass(frozen=True)
class ConfigResourceListResponse:
    status: ConfigServiceStatus
    message: str
    resources: tuple[ConfigResourceDescriptor, ...] = ()


@dataclasses.dataclass(frozen=True)
class ConfigResourceDescribeRequest:
    resource_id: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceDescribeResponse:
    status: ConfigServiceStatus
    message: str
    resource: ConfigResourceDescriptor
    fields: tuple[ConfigFieldSchema, ...] = ()


@dataclasses.dataclass(frozen=True)
class ConfigResourceGetRequest:
    resource_id: str
    variant_id: str = ""


@dataclasses.dataclass(frozen=True)
class ConfigResourceGetResponse:
    status: ConfigServiceStatus
    message: str
    snapshot: ConfigSnapshot


@dataclasses.dataclass(frozen=True)
class ConfigResourcePatchRequest:
    resource_id: str
    expected_revision: str
    updates: tuple[ConfigFieldValue, ...]
    variant_id: str = ""
    validate_only: bool = False


@dataclasses.dataclass(frozen=True)
class ConfigResourcePatchResponse:
    """A configuration patch result.

    ``snapshot`` remains the persisted base snapshot. For a dry-run,
    ``candidate_snapshot`` is a provider-normalized preview only: save the exact
    updates again with ``snapshot.revision``, never with the candidate revision.
    """

    status: ConfigServiceStatus
    message: str
    snapshot: ConfigSnapshot
    errors: tuple[ConfigFieldError, ...] = ()
    effects: ConfigEffects = ConfigEffects()
    candidate_snapshot: ConfigSnapshot | None = None


@dataclasses.dataclass(frozen=True)
class ConfigResourceApplyRequest:
    resource_id: str
    expected_revision: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceApplyResponse:
    status: ConfigServiceStatus
    message: str
    applied_revision: str
    outcome: ConfigApplyOutcome
    effects: ConfigEffects = ConfigEffects()


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantListRequest:
    resource_id: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantListResponse:
    status: ConfigServiceStatus
    message: str
    active_variant_id: str
    variants: tuple[ConfigVariantDescriptor, ...] = ()


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantCreateRequest:
    resource_id: str
    variant_id: str
    source: ConfigVariantSource
    expected_active_revision: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantCreateResponse:
    status: ConfigServiceStatus
    message: str
    variant: ConfigVariantDescriptor
    effects: ConfigEffects = ConfigEffects()


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantSaveCurrentRequest:
    resource_id: str
    variant_id: str
    expected_variant_revision: str
    expected_active_revision: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantSaveCurrentResponse:
    status: ConfigServiceStatus
    message: str
    variant: ConfigVariantDescriptor
    effects: ConfigEffects = ConfigEffects()


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantActivateRequest:
    resource_id: str
    variant_id: str
    expected_active_revision: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantActivateResponse:
    status: ConfigServiceStatus
    message: str
    applied_revision: str
    outcome: ConfigApplyOutcome
    effects: ConfigEffects = ConfigEffects()


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantDeleteRequest:
    resource_id: str
    variant_id: str
    expected_revision: str


@dataclasses.dataclass(frozen=True)
class ConfigResourceVariantDeleteResponse:
    status: ConfigServiceStatus
    message: str
