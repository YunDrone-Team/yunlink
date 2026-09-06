import math

from .com.yundrone.sunray.v2 import sunray_pb2 as sunray
from .org.yunlink.telemetry.v1 import telemetry_pb2 as telemetry
from .org.yunlink.system.v1 import system_pb2 as system
from .validation_media import (
    validate_camera_catalog_snapshot,
    validate_camera_descriptor,
    validate_camera_start_rtsp_response,
    validate_media_asset_chunk,
    validate_media_asset_item,
    validate_media_asset_list_request,
    validate_media_asset_list_response,
    validate_media_asset_ref,
)
from .validation_shell import (
    validate_shell_close_request,
    validate_shell_open_request,
    validate_shell_resize_request,
    validate_shell_write_request,
)

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
        if response.previous_unix_time_ms > (2**63 - 1) or not valid(response.applied_unix_time_ms):
            raise ValueError("clock sync response timestamps are invalid")
        if response.delta_ms != response.applied_unix_time_ms - response.previous_unix_time_ms:
            raise ValueError("clock sync response delta is invalid")
    elif response.previous_unix_time_ms or response.applied_unix_time_ms or response.delta_ms:
        raise ValueError("failed clock sync response contains timestamps")

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
        or state.controller_type not in {
            sunray.ACTIVE_CONTROLLER_UNKNOWN,
            sunray.ACTIVE_CONTROLLER_PX4,
            sunray.ACTIVE_CONTROLLER_SO3,
            sunray.ACTIVE_CONTROLLER_MPC,
            sunray.ACTIVE_CONTROLLER_NMPC,
        }
    ):
        raise ValueError("flight control state is invalid")


def validate_ugv_control_state(state: sunray.UgvControlState) -> None:
    if (
        not math.isfinite(state.battery_voltage_v)
        or state.battery_voltage_v < 0
        or state.battery_percent > 100
    ):
        raise ValueError("UGV control state is invalid")


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


def validate_ugv_nav_goal(goal: sunray.UgvNavGoal) -> None:
    if not (
        goal.frame_id
        and goal.HasField("position_m")
        and _finite_vector(goal.position_m)
        and math.isfinite(goal.yaw_rad)
    ):
        raise ValueError("UGV navigation goal is invalid")


def validate_ugv_waypoint_mission_goal(goal: sunray.UgvWaypointMissionGoal) -> None:
    if not goal.frame_id:
        raise ValueError("UGV waypoint frame is missing")
    if not goal.task_name or len(goal.task_name.encode()) > 96:
        raise ValueError("UGV waypoint task name is invalid")
    if goal.completion_action not in {
        sunray.UGV_MISSION_HOLD,
        sunray.UGV_MISSION_RETURN_HOME_AND_HOLD,
    }:
        raise ValueError("UGV waypoint completion action is invalid")
    if not 1 <= len(goal.waypoints) <= 256:
        raise ValueError("UGV waypoint count is invalid")
    for waypoint in goal.waypoints:
        if not (
            waypoint.HasField("position_m")
            and _finite_vector(waypoint.position_m)
            and math.isfinite(waypoint.yaw_rad)
            and math.isfinite(waypoint.hold_time_s)
            and waypoint.hold_time_s >= 0
            and waypoint.arrival_action
            in {
                sunray.UGV_WAYPOINT_NEXT,
                sunray.UGV_WAYPOINT_HOLD_CURRENT_YAW,
                sunray.UGV_WAYPOINT_HOLD_SET_YAW,
            }
        ):
            raise ValueError("UGV waypoint is invalid")


def validate_ugv_planning_state(state: sunray.UgvPlanningState) -> None:
    current_valid = not state.HasField("current_waypoint") or (
        state.current_waypoint.HasField("position_m")
        and _finite_vector(state.current_waypoint.position_m)
        and math.isfinite(state.current_waypoint.yaw_rad)
        and math.isfinite(state.current_waypoint.hold_time_s)
    )
    if not (
        0 <= state.main_state <= 3
        and 0 <= state.task_state <= 5
        and state.current_waypoint_index <= state.total_waypoints
        and math.isfinite(state.distance_to_goal_m)
        and state.distance_to_goal_m >= 0
        and math.isfinite(state.hold_remaining_s)
        and state.hold_remaining_s >= 0
        and current_valid
    ):
        raise ValueError("UGV planning state is invalid")


from .validation_sunray import (
    validate_formation_leader_target_request,
    validate_formation_set_request,
    validate_formation_state,
    validate_gimbal_angle_goal,
    validate_gimbal_rate_goal,
    validate_gimbal_zoom_absolute_goal,
    validate_mapping_state,
)
