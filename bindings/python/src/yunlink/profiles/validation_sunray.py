import math

from .com.yundrone.sunray.v2 import sunray_pb2 as sunray


def _finite(value) -> bool:
    return isinstance(value, (int, float)) and math.isfinite(value)


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


def _valid_double_ring(request: sunray.FormationSetRequest, dynamic: bool) -> bool:
    if not request.HasField("double_ring"):
        return False
    ring = request.double_ring
    if not (
        math.isfinite(ring.radius_m)
        and ring.radius_m > 0
        and math.isfinite(ring.lower_height_m)
        and math.isfinite(ring.upper_height_m)
        and ring.lower_height_m < ring.upper_height_m
        and math.isfinite(ring.phase_offset_rad)
    ):
        return False
    # STATIC_DOUBLE_RING ignores angular speed entirely, so an unused non-finite
    # value stays acceptable; the dynamic variant requires finite non-zero motion.
    if dynamic and not (
        math.isfinite(ring.angular_speed_radps) and abs(ring.angular_speed_radps) > 0
    ):
        return False
    return True


def validate_formation_set_request(request: sunray.FormationSetRequest) -> None:
    positive = lambda value: math.isfinite(value) and value > 0
    moving = lambda value: math.isfinite(value) and abs(value) > 0
    formation_type = request.formation_type
    # Height semantics are part of the task contract: an unsupported mode is
    # rejected for every formation type, even one that ignores planar height.
    if request.height_mode not in {
        sunray.FORMATION_HEIGHT_LEGACY_HOLD_CURRENT,
        sunray.FORMATION_HEIGHT_EXPLICIT,
    } or not math.isfinite(request.height_m):
        raise ValueError("formation request is invalid")
    valid = False
    if formation_type in {sunray.FORMATION_TAKEOFF, sunray.FORMATION_LAND}:
        valid = (
            request.height_mode == sunray.FORMATION_HEIGHT_LEGACY_HOLD_CURRENT
            and request.height_m == 0
        )
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
    elif formation_type == sunray.FORMATION_STATIC_DOUBLE_RING:
        valid = _valid_double_ring(request, False)
    elif formation_type == sunray.FORMATION_DYNAMIC_DOUBLE_RING:
        valid = _valid_double_ring(request, True)
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
        sunray.FORMATION_STATIC_DOUBLE_RING,
        sunray.FORMATION_DYNAMIC_POLYGON,
        sunray.FORMATION_DYNAMIC_RING,
        sunray.FORMATION_DYNAMIC_LEMNISCATE,
        sunray.FORMATION_DYNAMIC_DOUBLE_RING,
    }
    valid_target = not state.virtual_leader_target_valid or (
        state.HasField("virtual_leader_target") and _finite_pose(state.virtual_leader_target)
    )
    # 2.10 起一并承载"本机已受理的编队槽位目标"：valid ⇒ 位姿存在且有限；
    # valid == false 时字段整体缺省是合法表达（"当前没有已受理目标"）。
    # 与 C++ 侧 validate_formation_state 同口径；需要随 proto 重新生成的 sunray_pb2.py 才带这两个字段。
    valid_formation_target = not state.formation_target_valid or (
        state.HasField("formation_target") and _finite_pose(state.formation_target)
    )
    if (
        not 0 <= state.phase <= 4
        or not 0 <= state.dynamic_start_status <= 4
        or not valid_type
        or not valid_target
        or not valid_formation_target
    ):
        raise ValueError("formation state is invalid")


def validate_formation_shape(shape: sunray.FormationShape) -> None:
    """2.11 动态阵型几何图形；与 C++ 侧 validate_formation_shape 同口径。"""
    if not shape.valid:
        # 清除语义：points 必须整体缺省，不允许"声明无效却带点"的含糊表达。
        if len(shape.points) != 0:
            raise ValueError("formation shape is invalid")
        return
    if shape.formation_type not in {
        sunray.FORMATION_DYNAMIC_POLYGON,
        sunray.FORMATION_DYNAMIC_RING,
        sunray.FORMATION_DYNAMIC_LEMNISCATE,
    }:
        raise ValueError("formation shape type is not dynamic planar")
    if len(shape.points) < 3:
        raise ValueError("formation shape needs at least 3 points")
    for point in shape.points:
        if not (_finite(point.x) and _finite(point.y) and _finite(point.z)):
            raise ValueError("formation shape has a non-finite point")
    if not _finite(shape.move_speed_mps):
        raise ValueError("formation shape move speed is not finite")


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
