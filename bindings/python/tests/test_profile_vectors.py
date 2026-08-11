from google.protobuf.message import DecodeError
import pytest

from yunlink.profiles import (
    mobility,
    sunray,
    telemetry,
    media,
    validate_camera_catalog_snapshot,
    validate_camera_descriptor,
    validate_media_asset_chunk,
    validate_media_asset_ref,
    validate_media_asset_item,
    validate_media_asset_list_request,
    validate_media_asset_list_response,
    validate_camera_start_rtsp_response,
    validate_flight_control_state,
    validate_summary_snapshot,
    validate_emergency_kill_goal,
    validate_land_goal,
    validate_takeoff_goal,
    validate_uav_direct_control_goal,
    validate_uav_waypoint_mission_goal,
    validate_planner_set_home_request,
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


def test_media_profile_matches_cross_language_golden_and_rejects_invalid_assets():
    request = media.CameraTakePhotoRequest(camera_uid="front")
    assert request.SerializeToString(deterministic=True) == bytes.fromhex("0a0566726f6e74")

    camera = media.CameraDescriptor(
        camera_uid="front", camera_id=1, name="Front camera", online=True,
        frame_rate_hz=30.0, live_view_supported=True, live_view_active=True,
        live_view_autostart=True,
        rtsp_url="rtsp://192.168.10.38:8554/front",
    )
    validate_camera_descriptor(camera)
    catalog = media.CameraCatalogSnapshot(
        generated_at_ns=42, cameras=[camera], camera_manager_available=True,
    )
    validate_camera_catalog_snapshot(catalog)
    catalog.cameras[0].rtsp_url = ""
    with pytest.raises(ValueError, match="camera descriptor is invalid"):
        validate_camera_catalog_snapshot(catalog)
    catalog.cameras[0].CopyFrom(camera)
    catalog.cameras.add().CopyFrom(camera)
    with pytest.raises(ValueError, match="duplicate camera uid"):
        validate_camera_catalog_snapshot(catalog)

    asset = media.MediaAssetRef(
        asset_id="asset-01", kind=media.MEDIA_PHOTO, mime_type="image/png",
        size_bytes=8, sha256=b"\x01" * 32, camera_uid="front",
    )
    validate_media_asset_ref(asset)
    asset.sha256 = b"\x01" * 31
    with pytest.raises(ValueError, match="media asset reference is invalid"):
        validate_media_asset_ref(asset)

    chunk = media.MediaAssetChunkResponse(
        error=media.MEDIA_OK, transfer_id="transfer-01", eof=True,
    )
    validate_media_asset_chunk(chunk)
    validate_media_asset_chunk(
        media.MediaAssetChunkResponse(error=media.MEDIA_BUSY, message="queue is full")
    )


def test_media_rtsp_start_response_preserves_the_provider_url_byte_for_byte():
    response = media.CameraStartRtspResponse(
        error=media.MEDIA_OK,
        message="ready",
        rtsp_url="rtsp://viewer:secret@192.168.10.60:8554/front/main?profile=high&token=a%2Fb",
    )
    validate_camera_start_rtsp_response(response)
    assert response.SerializeToString(deterministic=True).hex() == (
        "0801120572656164791a4b727473703a2f2f7669657765723a736563726574403139322e"
        "3136382e31302e36303a383535342f66726f6e742f6d61696e3f70726f66696c653d6869"
        "676826746f6b656e3d6125324662"
    )

    response.rtsp_url = ""
    with pytest.raises(ValueError, match="camera start RTSP response is invalid"):
        validate_camera_start_rtsp_response(response)
    response.error = media.MEDIA_OPERATION_FAILED
    response.message = "provider rejected start"
    validate_camera_start_rtsp_response(response)


def test_media_asset_list_contract_is_paged_and_keeps_thumbnail_relation_explicit():
    request = media.MediaAssetListRequest(
        camera_uid="front", kinds=[media.MEDIA_PHOTO, media.MEDIA_VIDEO],
        created_after_ns=10, created_before_ns=20, page_size=25, page_token="Y3Vyc29y",
    )
    validate_media_asset_list_request(request)
    assert request.SerializeToString(deterministic=True).hex() == (
        "0a0566726f6e7412020103180a2014281932085933567963323979"
    )
    asset = media.MediaAssetRef(
        asset_id="photo-1", kind=media.MEDIA_PHOTO, mime_type="image/png", size_bytes=8,
        sha256=b"\x01" * 32, created_at_ns=42, camera_uid="front", display_name="photo.png",
    )
    thumbnail = media.MediaAssetRef(
        asset_id="thumb-1", kind=media.MEDIA_THUMBNAIL, mime_type="image/png", size_bytes=4,
        sha256=b"\x02" * 32, created_at_ns=42, camera_uid="front", display_name="thumb.png",
    )
    response = media.MediaAssetListResponse(
        error=media.MEDIA_OK, items=[media.MediaAssetItem(
            asset=asset, thumbnail=thumbnail, width=1920, height=1080,
        )], next_page_token="next", catalog_revision=7,
    )
    validate_media_asset_item(response.items[0])
    validate_media_asset_list_response(response)
    request.page_size = 101
    with pytest.raises(ValueError, match="media asset list request is invalid"):
        validate_media_asset_list_request(request)


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
    )
    validate_flight_control_state(state)
    assert sunray.FlightControlState.FromString(state.SerializeToString()) == state
    state.battery_percent = 101
    with pytest.raises(ValueError, match="flight control state is invalid"):
        validate_flight_control_state(state)


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
