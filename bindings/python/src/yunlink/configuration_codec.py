"""Deterministic little-endian codecs for YunLink Configuration payloads."""

from __future__ import annotations

import math
import struct
from collections.abc import Callable
from typing import Any, TypeVar

from .configuration import *

MAX_CONFIG_ITEMS = 256
MAX_STRING_BYTES = 1024
T = TypeVar("T")


class ConfigurationCodecError(ValueError):
    """A payload is malformed, non-canonical, or exceeds the public contract."""


class _Writer:
    def __init__(self) -> None:
        self.data = bytearray()

    def u8(self, value: int) -> None:
        self.data.append(value)

    def boolean(self, value: bool) -> None:
        self.u8(int(value))

    def u16(self, value: int) -> None:
        self.data.extend(struct.pack("<H", value))

    def u64(self, value: int) -> None:
        self.data.extend(struct.pack("<Q", value))

    def f64(self, value: float) -> None:
        if not math.isfinite(value):
            raise ConfigurationCodecError("non-finite double")
        self.data.extend(struct.pack("<d", value))

    def text(self, value: str) -> None:
        raw = value.encode("utf-8")
        if len(raw) > MAX_STRING_BYTES:
            raise ConfigurationCodecError("string exceeds configuration contract")
        self.u16(len(raw))
        self.data.extend(raw)

    def items(self, values: tuple[T, ...] | list[T], write: Callable[[T], None]) -> None:
        if len(values) > MAX_CONFIG_ITEMS:
            raise ConfigurationCodecError("list exceeds configuration contract")
        self.u16(len(values))
        for value in values:
            write(value)


class _Reader:
    def __init__(self, data: bytes) -> None:
        self.data = data
        self.cursor = 0

    def _take(self, size: int) -> bytes:
        value = self.data[self.cursor : self.cursor + size]
        if len(value) != size:
            raise ConfigurationCodecError("truncated configuration payload")
        self.cursor += size
        return value

    def u8(self) -> int:
        return self._take(1)[0]

    def boolean(self) -> bool:
        value = self.u8()
        if value not in (0, 1):
            raise ConfigurationCodecError("invalid boolean")
        return bool(value)

    def u16(self) -> int:
        return struct.unpack("<H", self._take(2))[0]

    def u64(self) -> int:
        return struct.unpack("<Q", self._take(8))[0]

    def f64(self) -> float:
        value = struct.unpack("<d", self._take(8))[0]
        if not math.isfinite(value):
            raise ConfigurationCodecError("non-finite double")
        return value

    def text(self) -> str:
        length = self.u16()
        if length > MAX_STRING_BYTES:
            raise ConfigurationCodecError("string exceeds configuration contract")
        try:
            return self._take(length).decode("utf-8")
        except UnicodeDecodeError as error:
            raise ConfigurationCodecError("invalid UTF-8") from error

    def items(self, read: Callable[[], T]) -> tuple[T, ...]:
        count = self.u16()
        if count > MAX_CONFIG_ITEMS:
            raise ConfigurationCodecError("list exceeds configuration contract")
        return tuple(read() for _ in range(count))


def _enum(enum_type: type[enum.IntEnum], value: int) -> enum.IntEnum:
    try:
        return enum_type(value)
    except ValueError as error:
        raise ConfigurationCodecError(f"invalid {enum_type.__name__}") from error


def _write_value(writer: _Writer, value: ConfigValue) -> None:
    writer.u8(value.value_type)
    if value.value_type is ConfigValueType.BOOL:
        if not isinstance(value.value, bool):
            raise ConfigurationCodecError("invalid bool ConfigValue")
        writer.boolean(value.value)
    elif value.value_type is ConfigValueType.INT64:
        if isinstance(value.value, bool) or not isinstance(value.value, int):
            raise ConfigurationCodecError("invalid int64 ConfigValue")
        writer.u64(value.value & ((1 << 64) - 1))
    elif value.value_type is ConfigValueType.DOUBLE:
        if not isinstance(value.value, float):
            raise ConfigurationCodecError("invalid double ConfigValue")
        writer.f64(value.value)
    elif value.value_type is ConfigValueType.STRING:
        if not isinstance(value.value, str):
            raise ConfigurationCodecError("invalid string ConfigValue")
        writer.text(value.value)
    elif value.value_type is ConfigValueType.STRING_LIST:
        if not isinstance(value.value, tuple) or not all(isinstance(item, str) for item in value.value):
            raise ConfigurationCodecError("invalid string-list ConfigValue")
        writer.items(value.value, writer.text)
    elif value.value_type is ConfigValueType.DOUBLE_LIST:
        if not isinstance(value.value, tuple) or not all(isinstance(item, float) for item in value.value):
            raise ConfigurationCodecError("invalid double-list ConfigValue")
        writer.items(value.value, writer.f64)
    else:
        raise ConfigurationCodecError("invalid ConfigValue type")


def _read_value(reader: _Reader) -> ConfigValue:
    value_type = _enum(ConfigValueType, reader.u8())
    if value_type is ConfigValueType.BOOL:
        return ConfigValue.bool(reader.boolean())
    if value_type is ConfigValueType.INT64:
        value = reader.u64()
        return ConfigValue.int64(value - (1 << 64) if value >= 1 << 63 else value)
    if value_type is ConfigValueType.DOUBLE:
        return ConfigValue.double(reader.f64())
    if value_type is ConfigValueType.STRING:
        return ConfigValue.string(reader.text())
    if value_type is ConfigValueType.STRING_LIST:
        return ConfigValue.string_list(reader.items(reader.text))
    return ConfigValue.double_list(reader.items(reader.f64))


def _write_descriptor(writer: _Writer, value: ConfigResourceDescriptor) -> None:
    writer.text(value.id); writer.text(value.title); writer.text(value.description)
    writer.boolean(value.readable); writer.boolean(value.writable)
    writer.boolean(value.apply_supported); writer.boolean(value.variants_supported)


def _read_descriptor(reader: _Reader) -> ConfigResourceDescriptor:
    return ConfigResourceDescriptor(reader.text(), reader.text(), reader.text(), reader.boolean(),
                                    reader.boolean(), reader.boolean(), reader.boolean())


def _write_schema(writer: _Writer, value: ConfigFieldSchema) -> None:
    writer.text(value.path); writer.text(value.title); writer.text(value.description)
    writer.u8(value.value_type); writer.boolean(value.required); writer.boolean(value.read_only)
    writer.boolean(value.sensitive); writer.boolean(value.minimum is not None)
    writer.f64(value.minimum if value.minimum is not None else 0.0)
    writer.boolean(value.maximum is not None)
    writer.f64(value.maximum if value.maximum is not None else 0.0)
    writer.text(value.validation_pattern)
    writer.items(value.choices, lambda item: (_write_value(writer, item.value), writer.text(item.label)))
    writer.text(value.group_path); writer.u8(value.update_policy); writer.text(value.unit)


def _read_schema(reader: _Reader) -> ConfigFieldSchema:
    path, title, description = reader.text(), reader.text(), reader.text()
    value_type = _enum(ConfigValueType, reader.u8())
    required, read_only, sensitive = reader.boolean(), reader.boolean(), reader.boolean()
    has_minimum, minimum = reader.boolean(), reader.f64()
    has_maximum, maximum = reader.boolean(), reader.f64()
    pattern = reader.text()
    choices = reader.items(lambda: ConfigChoice(_read_value(reader), reader.text()))
    return ConfigFieldSchema(path, title, description, value_type, required, read_only, sensitive,
                             minimum if has_minimum else None, maximum if has_maximum else None,
                             pattern, choices, reader.text(),
                             _enum(ConfigFieldUpdatePolicy, reader.u8()), reader.text())


def _write_field_value(writer: _Writer, value: ConfigFieldValue) -> None:
    writer.text(value.path); _write_value(writer, value.value)


def _read_field_value(reader: _Reader) -> ConfigFieldValue:
    return ConfigFieldValue(reader.text(), _read_value(reader))


def _write_snapshot(writer: _Writer, value: ConfigSnapshot) -> None:
    writer.text(value.resource_id); writer.text(value.revision); writer.text(value.applied_revision)
    writer.text(value.variant_id); writer.text(value.active_variant_id)
    writer.items(value.values, lambda item: _write_field_value(writer, item))


def _read_snapshot(reader: _Reader) -> ConfigSnapshot:
    return ConfigSnapshot(reader.text(), reader.text(), reader.text(), reader.text(), reader.text(),
                          reader.items(lambda: _read_field_value(reader)))


def _write_status(writer: _Writer, status: ConfigServiceStatus, message: str) -> None:
    writer.u8(status); writer.text(message)


def _read_status(reader: _Reader) -> tuple[ConfigServiceStatus, str]:
    return _enum(ConfigServiceStatus, reader.u8()), reader.text()


def _write_effects(writer: _Writer, value: ConfigEffects) -> None:
    writer.u8(value.requirement); writer.items(value.affected_components, writer.text)
    writer.boolean(value.reconnect_expected)


def _read_effects(reader: _Reader) -> ConfigEffects:
    return ConfigEffects(_enum(ConfigApplyRequirement, reader.u8()), reader.items(reader.text), reader.boolean())


def _write_variant(writer: _Writer, value: ConfigVariantDescriptor) -> None:
    writer.text(value.id); writer.text(value.title); writer.text(value.revision); writer.u64(value.modified_at_ns)
    writer.boolean(value.active); writer.boolean(value.mutable_variant)


def _read_variant(reader: _Reader) -> ConfigVariantDescriptor:
    return ConfigVariantDescriptor(reader.text(), reader.text(), reader.text(), reader.u64(),
                                   reader.boolean(), reader.boolean())


def _write_error(writer: _Writer, value: ConfigFieldError) -> None:
    writer.text(value.path); writer.text(value.code); writer.text(value.message)


def _read_error(reader: _Reader) -> ConfigFieldError:
    return ConfigFieldError(reader.text(), reader.text(), reader.text())


def encode(payload: Any) -> bytes:
    writer = _Writer()
    if isinstance(payload, ConfigResourceListRequest): writer.u8(0)
    elif isinstance(payload, ConfigResourceListResponse):
        _write_status(writer, payload.status, payload.message); writer.items(payload.resources, lambda item: _write_descriptor(writer, item))
    elif isinstance(payload, ConfigResourceDescribeRequest): writer.text(payload.resource_id)
    elif isinstance(payload, ConfigResourceDescribeResponse):
        _write_status(writer, payload.status, payload.message); _write_descriptor(writer, payload.resource); writer.items(payload.fields, lambda item: _write_schema(writer, item))
    elif isinstance(payload, ConfigResourceGetRequest): writer.text(payload.resource_id); writer.text(payload.variant_id)
    elif isinstance(payload, ConfigResourceGetResponse): _write_status(writer, payload.status, payload.message); _write_snapshot(writer, payload.snapshot)
    elif isinstance(payload, ConfigResourcePatchRequest):
        writer.text(payload.resource_id); writer.text(payload.variant_id); writer.text(payload.expected_revision); writer.items(payload.updates, lambda item: _write_field_value(writer, item)); writer.boolean(payload.validate_only)
    elif isinstance(payload, ConfigResourcePatchResponse):
        _write_status(writer, payload.status, payload.message); _write_snapshot(writer, payload.snapshot); writer.boolean(payload.candidate_snapshot is not None)
        if payload.candidate_snapshot is not None: _write_snapshot(writer, payload.candidate_snapshot)
        writer.items(payload.errors, lambda item: _write_error(writer, item)); _write_effects(writer, payload.effects)
    elif isinstance(payload, ConfigResourceApplyRequest): writer.text(payload.resource_id); writer.text(payload.expected_revision)
    elif isinstance(payload, ConfigResourceApplyResponse):
        _write_status(writer, payload.status, payload.message); writer.text(payload.applied_revision); writer.u8(payload.outcome); _write_effects(writer, payload.effects)
    elif isinstance(payload, ConfigResourceVariantListRequest): writer.text(payload.resource_id)
    elif isinstance(payload, ConfigResourceVariantListResponse):
        _write_status(writer, payload.status, payload.message); writer.text(payload.active_variant_id); writer.items(payload.variants, lambda item: _write_variant(writer, item))
    elif isinstance(payload, ConfigResourceVariantCreateRequest):
        writer.text(payload.resource_id); writer.text(payload.variant_id); writer.u8(payload.source); writer.text(payload.expected_active_revision)
    elif isinstance(payload, (ConfigResourceVariantCreateResponse, ConfigResourceVariantSaveCurrentResponse)):
        _write_status(writer, payload.status, payload.message); _write_variant(writer, payload.variant); _write_effects(writer, payload.effects)
    elif isinstance(payload, ConfigResourceVariantSaveCurrentRequest):
        writer.text(payload.resource_id); writer.text(payload.variant_id); writer.text(payload.expected_variant_revision); writer.text(payload.expected_active_revision)
    elif isinstance(payload, ConfigResourceVariantActivateRequest):
        writer.text(payload.resource_id); writer.text(payload.variant_id); writer.text(payload.expected_active_revision)
    elif isinstance(payload, ConfigResourceVariantActivateResponse):
        _write_status(writer, payload.status, payload.message); writer.text(payload.applied_revision); writer.u8(payload.outcome); _write_effects(writer, payload.effects)
    elif isinstance(payload, ConfigResourceVariantDeleteRequest):
        writer.text(payload.resource_id); writer.text(payload.variant_id); writer.text(payload.expected_revision)
    elif isinstance(payload, ConfigResourceVariantDeleteResponse): _write_status(writer, payload.status, payload.message)
    else: raise TypeError(f"unsupported Configuration payload: {type(payload)!r}")
    return bytes(writer.data)


def decode(payload_type: type[T], data: bytes) -> T:
    reader = _Reader(data)
    status: ConfigServiceStatus; message: str
    if payload_type is ConfigResourceListRequest:
        if reader.u8() != 0:
            raise ConfigurationCodecError("invalid list request")
        value: Any = ConfigResourceListRequest()
    elif payload_type is ConfigResourceListResponse:
        status, message = _read_status(reader); value = ConfigResourceListResponse(status, message, reader.items(lambda: _read_descriptor(reader)))
    elif payload_type is ConfigResourceDescribeRequest: value = ConfigResourceDescribeRequest(reader.text())
    elif payload_type is ConfigResourceDescribeResponse:
        status, message = _read_status(reader); value = ConfigResourceDescribeResponse(status, message, _read_descriptor(reader), reader.items(lambda: _read_schema(reader)))
    elif payload_type is ConfigResourceGetRequest: value = ConfigResourceGetRequest(reader.text(), reader.text())
    elif payload_type is ConfigResourceGetResponse:
        status, message = _read_status(reader); value = ConfigResourceGetResponse(status, message, _read_snapshot(reader))
    elif payload_type is ConfigResourcePatchRequest:
        resource_id = reader.text()
        variant_id = reader.text()
        expected_revision = reader.text()
        value = ConfigResourcePatchRequest(
            resource_id,
            expected_revision,
            reader.items(lambda: _read_field_value(reader)),
            variant_id,
            reader.boolean(),
        )
    elif payload_type is ConfigResourcePatchResponse:
        status, message = _read_status(reader); snapshot = _read_snapshot(reader); candidate_snapshot = _read_snapshot(reader) if reader.boolean() else None; value = ConfigResourcePatchResponse(status=status, message=message, snapshot=snapshot, errors=reader.items(lambda: _read_error(reader)), effects=_read_effects(reader), candidate_snapshot=candidate_snapshot)
    elif payload_type is ConfigResourceApplyRequest: value = ConfigResourceApplyRequest(reader.text(), reader.text())
    elif payload_type is ConfigResourceApplyResponse:
        status, message = _read_status(reader); value = ConfigResourceApplyResponse(status, message, reader.text(), _enum(ConfigApplyOutcome, reader.u8()), _read_effects(reader))
    elif payload_type is ConfigResourceVariantListRequest: value = ConfigResourceVariantListRequest(reader.text())
    elif payload_type is ConfigResourceVariantListResponse:
        status, message = _read_status(reader); value = ConfigResourceVariantListResponse(status, message, reader.text(), reader.items(lambda: _read_variant(reader)))
    elif payload_type is ConfigResourceVariantCreateRequest: value = ConfigResourceVariantCreateRequest(reader.text(), reader.text(), _enum(ConfigVariantSource, reader.u8()), reader.text())
    elif payload_type in (ConfigResourceVariantCreateResponse, ConfigResourceVariantSaveCurrentResponse):
        status, message = _read_status(reader); args = (status, message, _read_variant(reader), _read_effects(reader)); value = payload_type(*args)
    elif payload_type is ConfigResourceVariantSaveCurrentRequest: value = ConfigResourceVariantSaveCurrentRequest(reader.text(), reader.text(), reader.text(), reader.text())
    elif payload_type is ConfigResourceVariantActivateRequest: value = ConfigResourceVariantActivateRequest(reader.text(), reader.text(), reader.text())
    elif payload_type is ConfigResourceVariantActivateResponse:
        status, message = _read_status(reader); value = ConfigResourceVariantActivateResponse(status, message, reader.text(), _enum(ConfigApplyOutcome, reader.u8()), _read_effects(reader))
    elif payload_type is ConfigResourceVariantDeleteRequest: value = ConfigResourceVariantDeleteRequest(reader.text(), reader.text(), reader.text())
    elif payload_type is ConfigResourceVariantDeleteResponse:
        status, message = _read_status(reader); value = ConfigResourceVariantDeleteResponse(status, message)
    else: raise TypeError(f"unsupported Configuration payload type: {payload_type!r}")
    if reader.cursor != len(data): raise ConfigurationCodecError("trailing configuration payload bytes")
    return value
