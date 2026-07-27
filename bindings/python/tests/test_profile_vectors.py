from google.protobuf.message import DecodeError
import pytest

from yunlink.profiles import (
    mobility,
    sunray,
    telemetry,
    validate_summary_snapshot,
    validate_uav_direct_control_goal,
    validate_uav_waypoint_mission_goal,
)


GOTO_GOLDEN = bytes.fromhex(
    "0a036d6170121b09000000000000f03f1100000000000000c0"
    "19000000000000e03f19000000000000d03f"
)
DIRECT_CONTROL_GOLDEN = bytes.fromhex(
    "1a5c0a036d6170121b09000000000000f03f1100000000000000c019000000000000e03f"
    "1a1b099a9999999999b93f119a9999999999c93f19333333333333d3bf221b097b14ae47"
    "e17a843f117b14ae47e17a943f19b81e85eb51b89e3f320b080111000000000000d03f38"
    "0140ee05"
)
WAYPOINT_GOLDEN = bytes.fromhex(
    "0a036d6170122f0a1b09000000000000f03f110000000000000040190000000000000840"
    "11000000000000e03f19000000000000f83f12260a1b09000000000000f0bf1100000000"
    "000000c019000000000000104011000000000000d0bf1801"
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


def _valid_direct_goal(target: str) -> sunray.UavDirectControlGoal:
    goal = sunray.UavDirectControlGoal(
        yaw=sunray.YawTarget(mode=sunray.UAV_YAW_KEEP),
        controller=sunray.UAV_CONTROLLER_DEFAULT,
    )
    if target == "world_position":
        goal.world_position.frame_id = "map"
        goal.world_position.position_m.z = 1.0
    elif target == "body_position":
        goal.body_position.body_xy_position_m.x = 1.0
        goal.body_position.fixed_height_m = 2.0
    elif target == "trajectory_setpoint":
        goal.trajectory_setpoint.frame_id = "map"
        goal.trajectory_setpoint.position_m.z = 1.0
        goal.trajectory_setpoint.velocity_mps.x = 0.1
        goal.trajectory_setpoint.acceleration_mps2.y = 0.1
        goal.lease_ms = 750
    elif target == "world_velocity":
        goal.world_velocity.frame_id = "map"
        goal.world_velocity.velocity_mps.x = 0.5
        goal.lease_ms = 250
    else:
        goal.body_velocity.body_xy_velocity_mps.y = 0.5
        goal.body_velocity.fixed_height_m = 2.0
        goal.lease_ms = 2000
    return goal


@pytest.mark.parametrize(
    "target",
    [
        "world_position",
        "body_position",
        "trajectory_setpoint",
        "world_velocity",
        "body_velocity",
    ],
)
def test_direct_control_variants_round_trip(target):
    goal = _valid_direct_goal(target)
    validate_uav_direct_control_goal(goal)
    decoded = sunray.UavDirectControlGoal.FromString(
        goal.SerializeToString(deterministic=True)
    )
    assert decoded.WhichOneof("target") == target


def test_direct_control_and_waypoint_golden_vectors():
    direct = sunray.UavDirectControlGoal(
        yaw=sunray.YawTarget(mode=sunray.UAV_YAW_SET_ANGLE, value=0.25),
        controller=sunray.UAV_CONTROLLER_POSITION,
        lease_ms=750,
    )
    direct.trajectory_setpoint.frame_id = "map"
    direct.trajectory_setpoint.position_m.CopyFrom(mobility.Vector3(x=1, y=-2, z=0.5))
    direct.trajectory_setpoint.velocity_mps.CopyFrom(
        mobility.Vector3(x=0.1, y=0.2, z=-0.3)
    )
    direct.trajectory_setpoint.acceleration_mps2.CopyFrom(
        mobility.Vector3(x=0.01, y=0.02, z=0.03)
    )
    validate_uav_direct_control_goal(direct)
    assert direct.SerializeToString(deterministic=True) == DIRECT_CONTROL_GOLDEN

    mission = sunray.UavWaypointMissionGoal(
        frame_id="map", interrupt_current_task=True
    )
    mission.waypoints.add(
        position_m=mobility.Vector3(x=1, y=2, z=3), yaw_rad=0.5, hold_time_s=1.5
    )
    mission.waypoints.add(
        position_m=mobility.Vector3(x=-1, y=-2, z=4), yaw_rad=-0.25
    )
    validate_uav_waypoint_mission_goal(mission)
    assert mission.SerializeToString(deterministic=True) == WAYPOINT_GOLDEN


def test_direct_control_and_waypoint_validation_failures():
    body = _valid_direct_goal("body_velocity")
    body.lease_ms = 249
    with pytest.raises(ValueError, match="body velocity target is invalid"):
        validate_uav_direct_control_goal(body)

    body.lease_ms = 750
    body.body_velocity.fixed_height_m = 0
    with pytest.raises(ValueError, match="body velocity target is invalid"):
        validate_uav_direct_control_goal(body)

    trajectory = _valid_direct_goal("trajectory_setpoint")
    trajectory.trajectory_setpoint.acceleration_mps2.z = float("nan")
    with pytest.raises(ValueError, match="trajectory setpoint target is invalid"):
        validate_uav_direct_control_goal(trajectory)

    mission = sunray.UavWaypointMissionGoal(frame_id="map")
    with pytest.raises(ValueError, match="waypoint count is invalid"):
        validate_uav_waypoint_mission_goal(mission)
    for _ in range(257):
        mission.waypoints.add(position_m=mobility.Vector3())
    with pytest.raises(ValueError, match="waypoint count is invalid"):
        validate_uav_waypoint_mission_goal(mission)


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
