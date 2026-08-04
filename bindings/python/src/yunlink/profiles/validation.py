import math

from .com.yundrone.sunray.v2 import sunray_pb2 as sunray
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
