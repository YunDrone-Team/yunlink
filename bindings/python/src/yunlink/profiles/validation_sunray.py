import math

from .com.yundrone.sunray.v2 import sunray_pb2 as sunray


def _finite_vector(value) -> bool:
    components = [value.x, value.y]
    if hasattr(value, "z"):
        components.append(value.z)
    return all(math.isfinite(component) for component in components)


def _finite_pose(pose) -> bool:
    if not pose.HasField("position") or not pose.HasField("orientation"):
        return False
    orientation = pose.orientation
    norm_squared = sum(
        value * value
        for value in (orientation.x, orientation.y, orientation.z, orientation.w)
    )
    return _finite_vector(pose.position) and math.isfinite(norm_squared) and norm_squared > 1e-12


def validate_formation_set_request(request: sunray.FormationSetRequest) -> None:
    positive = lambda value: math.isfinite(value) and value > 0
    moving = lambda value: math.isfinite(value) and abs(value) > 0
    formation_type = request.formation_type
    valid = False
    if formation_type in {sunray.FORMATION_TAKEOFF, sunray.FORMATION_LAND}:
        valid = True
    elif formation_type == sunray.FORMATION_STATIC_LINE:
        valid = (
            request.HasField("line")
            and positive(request.line.spacing_m)
            and math.isfinite(request.line.angle_deg)
        )
    elif formation_type == sunray.FORMATION_STATIC_POLYGON:
        valid = request.HasField("polygon") and positive(request.polygon.side_length_m)
    elif formation_type == sunray.FORMATION_DYNAMIC_POLYGON:
        valid = (
            request.HasField("polygon")
            and positive(request.polygon.side_length_m)
            and moving(request.polygon.move_speed_mps)
        )
    elif formation_type == sunray.FORMATION_DYNAMIC_RING:
        valid = (
            request.HasField("ring")
            and positive(request.ring.radius_m)
            and moving(request.ring.move_speed_mps)
        )
    elif formation_type == sunray.FORMATION_DYNAMIC_LEMNISCATE:
        valid = (
            request.HasField("lemniscate")
            and positive(request.lemniscate.x_scale_m)
            and positive(request.lemniscate.y_scale_m)
            and moving(request.lemniscate.move_speed_mps)
        )
    elif formation_type == sunray.FORMATION_LEADER and request.HasField("leader"):
        agents = [slot for slot in request.leader.agent_slots if slot]
        valid = (
            len(request.leader.agent_slots) == 25
            and len(request.leader.virtual_leader_slots) == 25
            and positive(request.leader.spacing_m)
            and bool(agents)
            and all(slot <= 255 for slot in agents)
            and len(set(agents)) == len(agents)
            and sum(request.leader.virtual_leader_slots) == 1
        )
    if not valid:
        raise ValueError("formation request is invalid")


def validate_formation_leader_target_request(
    request: sunray.FormationLeaderTargetRequest,
) -> None:
    if request.target_mode == sunray.FORMATION_LEADER_TARGET_FIXED_POSE:
        valid = request.frame_id and request.HasField("target_pose") and _finite_pose(request.target_pose)
    elif request.target_mode == sunray.FORMATION_LEADER_TARGET_ODOM_TOPIC:
        valid = len(request.odom_topic) > 1 and request.odom_topic.startswith("/")
    else:
        valid = False
    if not valid:
        raise ValueError("formation leader target request is invalid")


def validate_formation_state(state: sunray.FormationState) -> None:
    valid_type = state.formation_type in {
        sunray.FORMATION_UNKNOWN,
        sunray.FORMATION_TAKEOFF,
        sunray.FORMATION_LAND,
        sunray.FORMATION_STATIC_LINE,
        sunray.FORMATION_STATIC_POLYGON,
        sunray.FORMATION_LEADER,
        sunray.FORMATION_DYNAMIC_POLYGON,
        sunray.FORMATION_DYNAMIC_RING,
        sunray.FORMATION_DYNAMIC_LEMNISCATE,
    }
    valid_target = not state.virtual_leader_target_valid or (
        state.HasField("virtual_leader_target") and _finite_pose(state.virtual_leader_target)
    )
    if not 0 <= state.phase <= 4 or not valid_type or not valid_target:
        raise ValueError("formation state is invalid")


def validate_mapping_state(state: sunray.MappingState) -> None:
    if state.status not in {"", "UNAVAILABLE", "IDLE", "ACCUMULATING", "ERROR"}:
        raise ValueError("mapping state status is invalid")
    for lidar in state.lidars:
        if not all(math.isfinite(value) for value in (
            lidar.lidar_rate_hz, lidar.imu_rate_hz, lidar.lidar_age_sec, lidar.imu_age_sec
        )):
            raise ValueError("mapping state contains a non-finite value")


def validate_gimbal_angle_goal(goal: sunray.GimbalAngleGoal) -> None:
    if not math.isfinite(goal.yaw_rad) or not math.isfinite(goal.pitch_rad):
        raise ValueError("gimbal angle goal is invalid")


def validate_gimbal_rate_goal(goal: sunray.GimbalRateGoal) -> None:
    if not -100 <= goal.yaw_control <= 100 or not -100 <= goal.pitch_control <= 100:
        raise ValueError("gimbal rate control must be between -100 and 100")


def validate_gimbal_zoom_absolute_goal(goal: sunray.GimbalZoomAbsoluteGoal) -> None:
    if not math.isfinite(goal.zoom) or not 1.0 <= goal.zoom <= 30.9:
        raise ValueError("gimbal zoom must be between 1.0 and 30.9")
