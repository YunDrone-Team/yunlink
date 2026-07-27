import math

from .org.yunlink.telemetry.v1 import telemetry_pb2 as telemetry


def valid_metric_key(key: str) -> bool:
    if not key or len(key.encode()) > 128:
        return False
    segments = key.split(".")
    return len(segments) >= 3 and all(
        segment
        and segment[0].isascii()
        and segment[0].islower()
        and segment[0].isalpha()
        and all(char.isascii() and (char.islower() or char.isdigit() or char == "_") for char in segment[1:])
        for segment in segments
    )


def validate_summary_snapshot(snapshot: telemetry.SummarySnapshot) -> None:
    if len(snapshot.metrics) > 64:
        raise ValueError("too many metrics")
    if snapshot.ByteSize() > 16 * 1024:
        raise ValueError("summary payload exceeds limit")
    keys: set[str] = set()
    for metric in snapshot.metrics:
        if not valid_metric_key(metric.key):
            raise ValueError("invalid metric key")
        if metric.key in keys:
            raise ValueError("duplicate metric key")
        keys.add(metric.key)
        if len(metric.unit.encode()) > 16:
            raise ValueError("metric unit exceeds limit")
        if metric.quality not in {
            telemetry.METRIC_VALID,
            telemetry.METRIC_STALE,
            telemetry.METRIC_INVALID,
            telemetry.METRIC_UNAVAILABLE,
        }:
            raise ValueError("invalid metric quality")
        value_kind = metric.value.WhichOneof("value") if metric.HasField("value") else None
        if metric.quality in {telemetry.METRIC_VALID, telemetry.METRIC_STALE} and value_kind is None:
            raise ValueError("metric value is required")
        if value_kind == "double_value" and not math.isfinite(metric.value.double_value):
            raise ValueError("metric double is not finite")
        if value_kind == "enum_token" and len(metric.value.enum_token.encode()) > 64:
            raise ValueError("metric enum token exceeds limit")
        if value_kind == "text_value" and len(metric.value.text_value.encode()) > 256:
            raise ValueError("metric text exceeds limit")
