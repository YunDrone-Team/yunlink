from google.protobuf.message import DecodeError
import pytest

from yunlink.profiles import (
    mobility,
    sunray,
    telemetry,
    system,
    validate_clock_sync_request,
    validate_clock_sync_response,
    validate_flight_control_state,
    validate_summary_snapshot,
    validate_emergency_kill_goal,
    validate_formation_leader_target_request,
    validate_formation_set_request,
    validate_formation_state,
    validate_land_goal,
    validate_takeoff_goal,
    validate_uav_direct_control_goal,
    validate_uav_waypoint_mission_goal,
    validate_ugv_move_point_goal,
    validate_ugv_velocity_goal,
    validate_planner_set_home_request,
    validate_gimbal_angle_goal,
    validate_gimbal_rate_goal,
    validate_gimbal_zoom_absolute_goal,
)
from yunlink.profiles.validation import (
    MINIMUM_TRUSTED_UNIX_TIME_MS,
    MAXIMUM_TRUSTED_UNIX_TIME_MS,
)


def test_system_clock_profile_matches_cross_language_golden_vectors():
    request = system.ClockSyncRequest(
        unix_time_ms=1_767_225_600_123, source="sunray-gcs"
    )
    validate_clock_sync_request(request)
    assert request.SerializeToString(deterministic=True).hex() == (
        "08fbd0eab6b733120a73756e7261792d676373"
    )

    status = system.ClockSyncResponse(
        error=system.CLOCK_SYNC_OK,
        message="synchronized",
        previous_unix_time_ms=1_767_225_600_000,
        applied_unix_time_ms=1_767_225_600_123,
        delta_ms=123,
    )
    validate_clock_sync_response(status)

    request.unix_time_ms = 1_000
    with pytest.raises(ValueError, match="clock sync request is invalid"):
        validate_clock_sync_request(request)


def test_system_clock_profile_rejects_inconsistent_response():
    with pytest.raises(ValueError, match="clock sync response"):
        validate_clock_sync_response(system.ClockSyncResponse(error=system.CLOCK_SYNC_OK))


def test_system_clock_profile_accepts_untrusted_previous_device_time():
    validate_clock_sync_response(system.ClockSyncResponse(
        error=system.CLOCK_SYNC_OK,
        message="synchronized",
        previous_unix_time_ms=31_449_600_000,
        applied_unix_time_ms=1_767_225_600_123,
        delta_ms=1_735_776_000_123,
    ))
    validate_clock_sync_response(system.ClockSyncResponse(
        error=system.CLOCK_SYNC_OK,
        message="synchronized",
        previous_unix_time_ms=0,
        applied_unix_time_ms=1_767_225_600_123,
        delta_ms=1_767_225_600_123,
    ))


def test_system_clock_profile_enforces_product_time_boundaries_and_source_token():
    for value in (MINIMUM_TRUSTED_UNIX_TIME_MS, MAXIMUM_TRUSTED_UNIX_TIME_MS):
        validate_clock_sync_request(system.ClockSyncRequest(
            unix_time_ms=value, source="sunray-gcs"
        ))
    for value in (MINIMUM_TRUSTED_UNIX_TIME_MS - 1, MAXIMUM_TRUSTED_UNIX_TIME_MS + 1):
        with pytest.raises(ValueError):
            validate_clock_sync_request(system.ClockSyncRequest(
                unix_time_ms=value, source="sunray-gcs"
            ))
    with pytest.raises(ValueError):
        validate_clock_sync_request(system.ClockSyncRequest(
            unix_time_ms=MINIMUM_TRUSTED_UNIX_TIME_MS, source="sunray gcs"
        ))


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


def test_flight_goal_validation_and_empty_payload_compatibility():
    takeoff = sunray.TakeoffGoal(takeoff_relative_height_m=1.2, takeoff_max_velocity_mps=0.5)
    validate_takeoff_goal(takeoff)
    assert sunray.TakeoffGoal.FromString(b"") == sunray.TakeoffGoal()
    with pytest.raises(ValueError, match="takeoff goal is invalid"):
        validate_takeoff_goal(sunray.TakeoffGoal(takeoff_relative_height_m=-1))
    validate_land_goal(sunray.LandGoal(land_max_velocity_mps=0.4))


def test_flight_control_state_round_trips_and_rejects_invalid_battery_values():
    state = sunray.FlightControlState(
        source_stamp_ns=42,
        armed=True,
        control_mode=1,
        control_state=3,
        battery_voltage_v=15.2,
        battery_percent=88,
        controller_type=sunray.ACTIVE_CONTROLLER_MPC,
    )
    validate_flight_control_state(state)
    assert sunray.FlightControlState.FromString(state.SerializeToString()) == state
    state.controller_type = 5
    with pytest.raises(ValueError, match="flight control state is invalid"):
        validate_flight_control_state(state)
    state.controller_type = sunray.ACTIVE_CONTROLLER_MPC
    state.battery_percent = 101
    with pytest.raises(ValueError, match="flight control state is invalid"):
        validate_flight_control_state(state)


def test_gimbal_v24_messages_match_cross_language_vectors_and_validate():
    state = sunray.GimbalParams(
        roll_rad=1.0,
        pitch_rad=-0.5,
        yaw_rad=0.25,
        zoom=3.5,
        connected=True,
        roll_rate_rad_s=0.1,
        pitch_rate_rad_s=-0.2,
        yaw_rate_rad_s=0.3,
    )
    assert state.SerializeToString(deterministic=True).hex() == (
        "11000000000000f03f19000000000000e0bf21000000000000d03f"
        "290000000000000c403801419a9999999999b93f499a9999999999c9bf"
        "51333333333333d33f"
    )

    angle = sunray.GimbalAngleGoal(yaw_rad=1.5707963267948966, pitch_rad=-0.7853981633974483)
    validate_gimbal_angle_goal(angle)
    assert angle.SerializeToString(deterministic=True).hex() == (
        "09182d4454fb21f93f11182d4454fb21e9bf"
    )
    rate = sunray.GimbalRateGoal(yaw_control=45, pitch_control=-30)
    validate_gimbal_rate_goal(rate)
    assert rate.SerializeToString(deterministic=True).hex() == (
        "082d10e2ffffffffffffffff01"
    )
    zoom = sunray.GimbalZoomAbsoluteGoal(zoom=3.5)
    validate_gimbal_zoom_absolute_goal(zoom)
    assert zoom.SerializeToString(deterministic=True).hex() == "090000000000000c40"
    assert sunray.GimbalCenterGoal.FromString(b"") == sunray.GimbalCenterGoal()
    with pytest.raises(ValueError, match="between -100 and 100"):
        validate_gimbal_rate_goal(sunray.GimbalRateGoal(yaw_control=101))


def test_ugv_v25_messages_match_cross_language_vectors_and_validate():
    move_goal = sunray.UgvMovePointGoal(
        frame=sunray.UGV_MOVE_LOCAL,
        yaw_mode=sunray.UGV_YAW_SET,
        desired_yaw_rad=0.25,
        local_frame_id="map",
    )
    move_goal.point_m.x = 1.0
    move_goal.point_m.y = -2.0
    validate_ugv_move_point_goal(move_goal)
    assert move_goal.SerializeToString(deterministic=True).hex() == (
        "121209000000000000f03f1100000000000000c0180121000000000000d03f2a036d6170"
    )
    move_goal.point_m.z = 0.1
    with pytest.raises(ValueError, match="UGV move point goal is invalid"):
        validate_ugv_move_point_goal(move_goal)

    velocity = sunray.UgvVelocityGoal(lease_ms=750)
    velocity.body.linear_mps.x = 0.5
    velocity.body.linear_mps.y = 0.1
    velocity.body.yaw_rate_radps = -0.2
    validate_ugv_velocity_goal(velocity)
    assert velocity.SerializeToString(deterministic=True).hex() == (
        "121d0a1209000000000000e03f119a9999999999b93f119a9999999999c9bf18ee05"
    )
    velocity.lease_ms = 249
    with pytest.raises(ValueError, match="UGV velocity lease is invalid"):
        validate_ugv_velocity_goal(velocity)

    state = sunray.UgvControlState(
        source_stamp_ns=42,
        drive_type=1,
        diagnostic_message="hold",
        fsm_state=1,
        odom_ready=True,
    )
    assert state.SerializeToString(deterministic=True).hex() == "082a28014a04686f6c6450017801"
    assert sunray.UgvHoldGoal().SerializeToString(deterministic=True) == b""


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

    home = sunray.PlannerSetHomeRequest(frame_id="map")
    home.home_m.x = 1.0
    home.home_m.y = -2.0
    home.home_m.z = 0.5
    validate_planner_set_home_request(home)
    assert home.SerializeToString(deterministic=True) == bytes.fromhex(
        "0a1b09000000000000f03f1100000000000000c019000000000000e03f12036d6170"
    )

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


def test_emergency_kill_requires_confirmation_and_matches_golden_vector():
    goal = sunray.EmergencyKillGoal()
    with pytest.raises(ValueError, match="emergency kill requires explicit confirmation"):
        validate_emergency_kill_goal(goal)
    goal.confirmed = True
    validate_emergency_kill_goal(goal)
    assert goal.SerializeToString(deterministic=True) == b"\x08\x01"
    assert sunray.EmergencyKillGoal.FromString(b"\x08\x01") == goal


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
        frame_id="map", task_name="yunlink-task-42",
        completion_action=sunray.UAV_MISSION_FINISH_HOVER,
    )
    mission.waypoints.add(
        position_m=mobility.Vector3(x=1, y=2, z=3), yaw_rad=0.5, hold_time_s=1.5,
        arrival_action=sunray.UAV_WAYPOINT_HOLD_SET_YAW,
    )
    mission.waypoints.add(
        position_m=mobility.Vector3(x=-1, y=-2, z=4), yaw_rad=-0.25,
        arrival_action=sunray.UAV_WAYPOINT_NEXT,
    )
    validate_uav_waypoint_mission_goal(mission)
    assert mission.SerializeToString(deterministic=True) == bytes.fromhex(
        "0a036d617012310a1b09000000000000f03f11000000000000004019000000000000084011000000000000e03f19000000000000f83f200212260a1b09000000000000f0bf1100000000000000c019000000000000104011000000000000d0bf1a0f79756e6c696e6b2d7461736b2d3432"
    )
    assert sunray.UavWaypointMissionGoal.FromString(mission.SerializeToString()) == mission


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

    mission = sunray.UavWaypointMissionGoal(frame_id="map", task_name="yunlink-empty")
    with pytest.raises(ValueError, match="waypoint count is invalid"):
        validate_uav_waypoint_mission_goal(mission)
    for _ in range(257):
        mission.waypoints.add(position_m=mobility.Vector3())
    with pytest.raises(ValueError, match="waypoint count is invalid"):
        validate_uav_waypoint_mission_goal(mission)


def test_formation_v27_messages_match_cross_language_vectors_and_validate():
    ring = sunray.FormationSetRequest(
        formation_type=sunray.FORMATION_DYNAMIC_RING,
        align_yaw_with_trajectory=True,
        ring=sunray.FormationRing(radius_m=3.0, move_speed_mps=-0.5),
    )
    validate_formation_set_request(ring)
    assert ring.SerializeToString(deterministic=True).hex() == (
        "081510012a1209000000000000084011000000000000e0bf"
    )

    variants = [
        sunray.FormationSetRequest(formation_type=sunray.FORMATION_TAKEOFF),
        sunray.FormationSetRequest(formation_type=sunray.FORMATION_LAND),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_STATIC_LINE,
            line=sunray.FormationLine(spacing_m=2, angle_deg=90),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_STATIC_POLYGON,
            polygon=sunray.FormationPolygon(side_length_m=2),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_DYNAMIC_POLYGON,
            polygon=sunray.FormationPolygon(side_length_m=2, move_speed_mps=0.5),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_DYNAMIC_LEMNISCATE,
            lemniscate=sunray.FormationLemniscate(
                x_scale_m=3, y_scale_m=2, move_speed_mps=0.5
            ),
        ),
        sunray.FormationSetRequest(
            formation_type=sunray.FORMATION_LEADER,
            leader=sunray.FormationLeader(
                agent_slots=[1, 2] + [0] * 23,
                spacing_m=2,
                virtual_leader_slots=[True] + [False] * 24,
            ),
        ),
    ]
    for request in variants:
        validate_formation_set_request(request)

    pose = mobility.Pose(
        position=mobility.Vector3(x=1, y=2, z=3),
        orientation=mobility.Quaternion(w=1),
    )
    target = sunray.FormationLeaderTargetRequest(
        target_mode=sunray.FORMATION_LEADER_TARGET_FIXED_POSE,
        source_stamp_ns=42,
        frame_id="map",
        target_pose=pose,
    )
    validate_formation_leader_target_request(target)
    assert target.SerializeToString(deterministic=True).hex() == (
        "0801102a1a036d617022280a1b09000000000000f03f110000000000000040"
        "190000000000000840120921000000000000f03f"
    )
    validate_formation_leader_target_request(
        sunray.FormationLeaderTargetRequest(
            target_mode=sunray.FORMATION_LEADER_TARGET_ODOM_TOPIC,
            odom_topic="/tracked/leader/odom",
        )
    )

    state = sunray.FormationState(
        source_stamp_ns=42,
        frame_id="map",
        agent_id="uav1",
        task_epoch=sunray.FormationTaskEpoch(
            domain_created_ns=10, origin_agent_id="uav1", origin_sequence=2
        ),
        phase=sunray.FORMATION_PHASE_ACTIVE,
        formation_type=sunray.FORMATION_LEADER,
        formation_established=True,
        virtual_leader_target_valid=True,
        virtual_leader_target=pose,
    )
    validate_formation_state(state)
    assert state.SerializeToString(deterministic=True).hex() == (
        "082a12036d61701a0475617631220a080a12047561763118023802400c48016001"
        "6a280a1b09000000000000f03f1100000000000000401900000000000008401209"
        "21000000000000f03f"
    )

    ring.ring.move_speed_mps = 0
    with pytest.raises(ValueError, match="formation request is invalid"):
        validate_formation_set_request(ring)


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
