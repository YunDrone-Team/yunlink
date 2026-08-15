import math

from .com.yundrone.sunray.v2 import sunray_pb2 as sunray
from .org.yunlink.telemetry.v1 import telemetry_pb2 as telemetry
from .org.yunlink.media.v1 import media_pb2 as media
from .org.yunlink.system.v1 import system_pb2 as system

MINIMUM_TRUSTED_UNIX_TIME_MS = 1_704_067_200_000
MAXIMUM_TRUSTED_UNIX_TIME_MS = 4_102_444_800_000


def _valid_clock_source(value: str) -> bool:
    return bool(value) and len(value.encode()) <= 64 and all(
        char.isascii() and (char.isalnum() or char in "-_.") for char in value
    )


def validate_clock_sync_request(request: system.ClockSyncRequest) -> None:
    if not (
        MINIMUM_TRUSTED_UNIX_TIME_MS
        <= request.unix_time_ms
        <= MAXIMUM_TRUSTED_UNIX_TIME_MS
        and _valid_clock_source(request.source)
    ):
        raise ValueError("clock sync request is invalid")


def validate_clock_sync_response(response: system.ClockSyncResponse) -> None:
    if response.error not in range(system.CLOCK_SYNC_OK, system.CLOCK_SYNC_INTERNAL_ERROR + 1):
        raise ValueError("clock sync response error is invalid")
    if len(response.message.encode()) > 256:
        raise ValueError("clock sync response message is too long")
    if response.error == system.CLOCK_SYNC_OK:
        valid = lambda value: MINIMUM_TRUSTED_UNIX_TIME_MS <= value <= MAXIMUM_TRUSTED_UNIX_TIME_MS
        if not valid(response.previous_unix_time_ms) or not valid(response.applied_unix_time_ms):
            raise ValueError("clock sync response timestamps are invalid")
        if response.delta_ms != response.applied_unix_time_ms - response.previous_unix_time_ms:
            raise ValueError("clock sync response delta is invalid")
    elif response.previous_unix_time_ms or response.applied_unix_time_ms or response.delta_ms:
        raise ValueError("failed clock sync response contains timestamps")


def _valid_media_token(value: str, max_bytes: int) -> bool:
    return (
        bool(value)
        and len(value.encode()) <= max_bytes
        and all(char.isascii() and (char.isalnum() or char in "-_.") for char in value)
    )


def _valid_media_page_token(value: str) -> bool:
    return len(value.encode()) <= 512 and all(
        char.isascii() and (char.isalnum() or char in "-_") or char == "=" for char in value
    )


def validate_camera_descriptor(camera: media.CameraDescriptor) -> None:
    if not (
        _valid_media_token(camera.camera_uid, 96)
        and len(camera.name.encode()) <= 128
        and len(camera.image_topic.encode()) <= 256
        and len(camera.camera_info_topic.encode()) <= 256
        and len(camera.encoding.encode()) <= 64
        and len(camera.error_message.encode()) <= 256
        and len(camera.rtsp_url.encode()) <= 2048
        and not any(ord(char) < 0x20 or ord(char) == 0x7F for char in camera.rtsp_url)
        and (
            not camera.live_view_active
            or (camera.live_view_supported and bool(camera.rtsp_url))
        )
        and (
            camera.live_view_supported
            or (
                not camera.live_view_control_supported
                and not camera.rtsp_url
                and not camera.live_view_autostart
            )
        )
        and math.isfinite(camera.frame_rate_hz)
        and camera.frame_rate_hz >= 0
    ):
        raise ValueError("camera descriptor is invalid")


def validate_camera_catalog_snapshot(snapshot: media.CameraCatalogSnapshot) -> None:
    if len(snapshot.cameras) > 32:
        raise ValueError("camera catalog is invalid")
    camera_uids: set[str] = set()
    for camera in snapshot.cameras:
        validate_camera_descriptor(camera)
        if camera.camera_uid in camera_uids:
            raise ValueError("duplicate camera uid")
        camera_uids.add(camera.camera_uid)


def validate_camera_start_rtsp_response(response: media.CameraStartRtspResponse) -> None:
    if not (
        response.error in set(range(media.MEDIA_OK, media.MEDIA_INTEGRITY_ERROR + 1))
        and len(response.message.encode()) <= 256
        and len(response.rtsp_url.encode()) <= 2048
        and not any(ord(char) < 0x20 or ord(char) == 0x7F for char in response.rtsp_url)
        and (
            (response.error == media.MEDIA_OK and bool(response.rtsp_url))
            or (response.error != media.MEDIA_OK and not response.rtsp_url)
        )
    ):
        raise ValueError("camera start RTSP response is invalid")


def validate_media_asset_ref(asset: media.MediaAssetRef) -> None:
    if not (
        _valid_media_token(asset.asset_id, 128)
        and asset.kind in {media.MEDIA_PHOTO, media.MEDIA_THUMBNAIL, media.MEDIA_VIDEO}
        and 0 < len(asset.mime_type.encode()) <= 96
        and asset.size_bytes > 0
        and len(asset.sha256) == 32
        and _valid_media_token(asset.camera_uid, 96)
        and len(asset.display_name.encode()) <= 160
    ):
        raise ValueError("media asset reference is invalid")


def validate_media_asset_item(item: media.MediaAssetItem) -> None:
    if not item.HasField("asset"):
        raise ValueError("media asset item is invalid")
    try:
        validate_media_asset_ref(item.asset)
    except ValueError as exc:
        raise ValueError("media asset item is invalid") from exc
    if item.asset.kind == media.MEDIA_THUMBNAIL or item.width > 32768 or item.height > 32768:
        raise ValueError("media asset item is invalid")
    if item.HasField("thumbnail"):
        try:
            validate_media_asset_ref(item.thumbnail)
        except ValueError as exc:
            raise ValueError("media asset thumbnail relation is invalid") from exc
        if (
            item.thumbnail.kind != media.MEDIA_THUMBNAIL
            or item.thumbnail.camera_uid != item.asset.camera_uid
            or item.thumbnail.asset_id == item.asset.asset_id
        ):
            raise ValueError("media asset thumbnail relation is invalid")


def validate_media_asset_list_request(request: media.MediaAssetListRequest) -> None:
    if not (
        (not request.camera_uid or _valid_media_token(request.camera_uid, 96))
        and 1 <= request.page_size <= 100
        and _valid_media_page_token(request.page_token)
        and not (
            request.created_after_ns
            and request.created_before_ns
            and request.created_after_ns > request.created_before_ns
        )
    ):
        raise ValueError("media asset list request is invalid")
    if len(set(request.kinds)) != len(request.kinds) or any(
        kind not in {media.MEDIA_PHOTO, media.MEDIA_VIDEO} for kind in request.kinds
    ):
        raise ValueError("media asset list kind is invalid")


def validate_media_asset_list_response(response: media.MediaAssetListResponse) -> None:
    if not (
        response.error in {
            media.MEDIA_OK, media.MEDIA_INVALID_REQUEST, media.MEDIA_CAMERA_UNAVAILABLE,
            media.MEDIA_UNSUPPORTED, media.MEDIA_BUSY, media.MEDIA_OPERATION_FAILED,
            media.MEDIA_TIMEOUT, media.MEDIA_INTERNAL_ERROR, media.MEDIA_PERMISSION_DENIED,
            media.MEDIA_NOT_FOUND, media.MEDIA_INTEGRITY_ERROR,
        }
        and len(response.message.encode()) <= 256
        and len(response.items) <= 100
        and _valid_media_page_token(response.next_page_token)
    ):
        raise ValueError("media asset list response is invalid")
    if response.error != media.MEDIA_OK and (response.items or response.next_page_token):
        raise ValueError("failed media asset list response contains data")
    asset_ids: set[str] = set()
    for item in response.items:
        try:
            validate_media_asset_item(item)
        except ValueError as exc:
            raise ValueError("media asset list response contains duplicate or invalid asset") from exc
        if item.asset.asset_id in asset_ids:
            raise ValueError("media asset list response contains duplicate or invalid asset")
        asset_ids.add(item.asset.asset_id)


def validate_media_asset_chunk(chunk: media.MediaAssetChunkResponse) -> None:
    if chunk.error not in {
        media.MEDIA_OK,
        media.MEDIA_INVALID_REQUEST,
        media.MEDIA_CAMERA_UNAVAILABLE,
        media.MEDIA_UNSUPPORTED,
        media.MEDIA_BUSY,
        media.MEDIA_OPERATION_FAILED,
        media.MEDIA_TIMEOUT,
        media.MEDIA_INTERNAL_ERROR,
        media.MEDIA_PERMISSION_DENIED,
        media.MEDIA_NOT_FOUND,
        media.MEDIA_INTEGRITY_ERROR,
    }:
        raise ValueError("media error is invalid")
    if not (
        len(chunk.message.encode()) <= 256
        and len(chunk.data) <= 256 * 1024
        and (not chunk.transfer_id or _valid_media_token(chunk.transfer_id, 128))
        and (
            chunk.error != media.MEDIA_OK
            or (bool(chunk.transfer_id) and (bool(chunk.data) or chunk.eof))
        )
    ):
        raise ValueError("media asset chunk is invalid")


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


def _finite_vector(value) -> bool:
    components = [value.x, value.y]
    if hasattr(value, "z"):
        components.append(value.z)
    return all(math.isfinite(component) for component in components)


def validate_flight_control_state(state: sunray.FlightControlState) -> None:
    if (
        not math.isfinite(state.battery_voltage_v)
        or state.battery_voltage_v < 0
        or state.battery_percent > 100
    ):
        raise ValueError("flight control state is invalid")


def validate_uav_direct_control_goal(goal: sunray.UavDirectControlGoal) -> None:
    valid_yaw = (
        goal.HasField("yaw")
        and goal.yaw.mode
        in {sunray.UAV_YAW_KEEP, sunray.UAV_YAW_SET_ANGLE, sunray.UAV_YAW_SET_RATE}
        and math.isfinite(goal.yaw.value)
    )
    if not valid_yaw:
        raise ValueError("yaw target is missing or invalid")
    if goal.controller not in {
        sunray.UAV_CONTROLLER_DEFAULT,
        sunray.UAV_CONTROLLER_POSITION,
        sunray.UAV_CONTROLLER_ATTITUDE,
    }:
        raise ValueError("controller is invalid")

    target = goal.WhichOneof("target")
    if target == "world_position":
        valid = (
            goal.lease_ms == 0
            and bool(goal.world_position.frame_id)
            and goal.world_position.HasField("position_m")
            and _finite_vector(goal.world_position.position_m)
        )
    elif target == "body_position":
        value = goal.body_position
        valid = (
            goal.lease_ms == 0
            and value.HasField("body_xy_position_m")
            and _finite_vector(value.body_xy_position_m)
            and math.isfinite(value.fixed_height_m)
            and value.fixed_height_m > 0
        )
    elif target == "trajectory_setpoint":
        value = goal.trajectory_setpoint
        valid = (
            250 <= goal.lease_ms <= 2000
            and bool(value.frame_id)
            and value.HasField("position_m")
            and value.HasField("velocity_mps")
            and value.HasField("acceleration_mps2")
            and _finite_vector(value.position_m)
            and _finite_vector(value.velocity_mps)
            and _finite_vector(value.acceleration_mps2)
        )
    elif target == "world_velocity":
        value = goal.world_velocity
        valid = (
            250 <= goal.lease_ms <= 2000
            and bool(value.frame_id)
            and value.HasField("velocity_mps")
            and _finite_vector(value.velocity_mps)
            and (
                not value.HasField("height_lock")
                or (math.isfinite(value.height_lock.height_m) and value.height_lock.height_m > 0)
            )
        )
    elif target == "body_velocity":
        value = goal.body_velocity
        valid = (
            250 <= goal.lease_ms <= 2000
            and value.HasField("body_xy_velocity_mps")
            and _finite_vector(value.body_xy_velocity_mps)
            and math.isfinite(value.fixed_height_m)
            and value.fixed_height_m > 0
        )
    else:
        raise ValueError("direct control target is missing")
    if not valid:
        raise ValueError(f"{target.replace('_', ' ')} target is invalid")


def validate_emergency_kill_goal(goal: sunray.EmergencyKillGoal) -> None:
    if not goal.confirmed:
        raise ValueError("emergency kill requires explicit confirmation")


def validate_takeoff_goal(goal: sunray.TakeoffGoal) -> None:
    if (
        not math.isfinite(goal.takeoff_relative_height_m)
        or goal.takeoff_relative_height_m < 0
        or not math.isfinite(goal.takeoff_max_velocity_mps)
        or goal.takeoff_max_velocity_mps < 0
    ):
        raise ValueError("takeoff goal is invalid")


def validate_land_goal(goal: sunray.LandGoal) -> None:
    if not math.isfinite(goal.land_max_velocity_mps) or goal.land_max_velocity_mps < 0:
        raise ValueError("land goal is invalid")


def validate_uav_waypoint_mission_goal(goal: sunray.UavWaypointMissionGoal) -> None:
    if not goal.frame_id:
        raise ValueError("waypoint frame is missing")
    if not goal.task_name or len(goal.task_name.encode()) > 96:
        raise ValueError("waypoint task name is invalid")
    if goal.completion_action not in {
        sunray.UAV_MISSION_FINISH_HOVER,
        sunray.UAV_MISSION_FINISH_RETURN_HOME_AND_LAND,
        sunray.UAV_MISSION_FINISH_LAND_NOW,
    }:
        raise ValueError("waypoint completion action is invalid")
    if not 1 <= len(goal.waypoints) <= 256:
        raise ValueError("waypoint count is invalid")
    for waypoint in goal.waypoints:
        if (
            not waypoint.HasField("position_m")
            or not _finite_vector(waypoint.position_m)
            or not math.isfinite(waypoint.yaw_rad)
            or not math.isfinite(waypoint.hold_time_s)
            or waypoint.hold_time_s < 0
            or waypoint.arrival_action
            not in {
                sunray.UAV_WAYPOINT_NEXT,
                sunray.UAV_WAYPOINT_HOLD_CURRENT_YAW,
                sunray.UAV_WAYPOINT_HOLD_SET_YAW,
            }
        ):
            raise ValueError("waypoint is invalid")


def validate_planner_set_home_request(request: sunray.PlannerSetHomeRequest) -> None:
    if (
        not request.frame_id
        or not request.HasField("home_m")
        or not _finite_vector(request.home_m)
    ):
        raise ValueError("Planner home request is invalid")


def validate_uav_nav_goal(goal: sunray.UavNavGoal) -> None:
    if (
        not goal.frame_id
        or not goal.HasField("position_m")
        or not _finite_vector(goal.position_m)
        or not math.isfinite(goal.yaw_rad)
    ):
        raise ValueError("UAV navigation goal is invalid")


def validate_ugv_move_point_goal(goal: sunray.UgvMovePointGoal) -> None:
    if not (
        goal.frame in {sunray.UGV_MOVE_LOCAL, sunray.UGV_MOVE_BODY}
        and goal.yaw_mode in {sunray.UGV_YAW_KEEP, sunray.UGV_YAW_SET}
        and goal.HasField("point_m")
        and _finite_vector(goal.point_m)
        and goal.point_m.z == 0
        and math.isfinite(goal.desired_yaw_rad)
        and ((goal.frame == sunray.UGV_MOVE_LOCAL) == bool(goal.local_frame_id))
    ):
        raise ValueError("UGV move point goal is invalid")


def validate_ugv_velocity_goal(goal: sunray.UgvVelocityGoal) -> None:
    if not 250 <= goal.lease_ms <= 2000:
        raise ValueError("UGV velocity lease is invalid")
    target = goal.WhichOneof("target")
    if target == "local":
        valid = (
            bool(goal.local.frame_id)
            and goal.local.HasField("linear_mps")
            and _finite_vector(goal.local.linear_mps)
            and math.isfinite(goal.local.desired_yaw_rad)
        )
    elif target == "body":
        valid = (
            goal.body.HasField("linear_mps")
            and _finite_vector(goal.body.linear_mps)
            and math.isfinite(goal.body.yaw_rate_radps)
        )
    else:
        valid = False
    if not valid:
        raise ValueError("UGV velocity target is invalid")


def validate_gimbal_angle_goal(goal: sunray.GimbalAngleGoal) -> None:
    if not math.isfinite(goal.yaw_rad) or not math.isfinite(goal.pitch_rad):
        raise ValueError("gimbal angle goal is invalid")


def validate_gimbal_rate_goal(goal: sunray.GimbalRateGoal) -> None:
    if not -100 <= goal.yaw_control <= 100 or not -100 <= goal.pitch_control <= 100:
        raise ValueError("gimbal rate control must be between -100 and 100")


def validate_gimbal_zoom_absolute_goal(goal: sunray.GimbalZoomAbsoluteGoal) -> None:
    if not math.isfinite(goal.zoom) or not 1.0 <= goal.zoom <= 30.9:
        raise ValueError("gimbal zoom must be between 1.0 and 30.9")
