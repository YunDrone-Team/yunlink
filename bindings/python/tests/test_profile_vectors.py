from google.protobuf.message import DecodeError
import pytest

from yunlink.profiles import mobility, sunray, telemetry, validate_summary_snapshot


GOTO_GOLDEN = bytes.fromhex(
    "0a036d6170121b09000000000000f03f1100000000000000c0"
    "19000000000000e03f19000000000000d03f"
)


def test_profile_payloads_match_cross_language_golden_vectors():
    goal = mobility.GotoGoal(frame_id="map", yaw_rad=0.25)
    goal.position.x = 1.0
    goal.position.y = -2.0
    goal.position.z = 0.5
    assert goal.SerializeToString(deterministic=True) == GOTO_GOLDEN

    request = sunray.FeatureStartRequest(name="mapping")
    assert request.SerializeToString(deterministic=True) == b"\x0a\x07mapping"

    with pytest.raises(DecodeError):
        sunray.FeatureStartRequest.FromString(b"\x0a\x08mapping")

    summary = telemetry.SummarySnapshot(generated_at_ns=1)
    metric = summary.metrics.add(
        key="org.test.ready",
        quality=telemetry.METRIC_VALID,
        source_timestamp_ns=2,
    )
    metric.value.bool_value = True
    validate_summary_snapshot(summary)
    assert summary.SerializeToString(deterministic=True) == bytes.fromhex(
        "080112180a0e6f72672e746573742e72656164791202080120012802"
    )

    summary.metrics.add().CopyFrom(metric)
    with pytest.raises(ValueError, match="duplicate metric key"):
        validate_summary_snapshot(summary)


@pytest.mark.parametrize(
    ("field", "value"),
    [
        ("bool_value", True),
        ("int_value", -7),
        ("double_value", 1.25),
        ("enum_token", "ready"),
        ("text_value", "diagnostic"),
    ],
)
def test_summary_validation_accepts_all_metric_value_types(field, value):
    summary = telemetry.SummarySnapshot()
    metric = summary.metrics.add(key="org.test.value", quality=telemetry.METRIC_VALID)
    setattr(metric.value, field, value)
    validate_summary_snapshot(summary)


def test_summary_validation_rejects_invalid_key_and_non_finite_double():
    summary = telemetry.SummarySnapshot()
    metric = summary.metrics.add(key="Org.test.value", quality=telemetry.METRIC_VALID)
    metric.value.double_value = 1.0
    with pytest.raises(ValueError, match="invalid metric key"):
        validate_summary_snapshot(summary)

    metric.key = "org.test.value"
    metric.value.double_value = float("nan")
    with pytest.raises(ValueError, match="metric double is not finite"):
        validate_summary_snapshot(summary)
