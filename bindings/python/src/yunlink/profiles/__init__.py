"""Generated optional YunLink Profile messages."""

MOBILITY_PROFILE_ID = "org.yunlink.mobility"
TELEMETRY_PROFILE_ID = "org.yunlink.telemetry"
SUNRAY_PROFILE_ID = "com.yundrone.sunray"

from .org.yunlink.mobility.v1 import mobility_pb2 as mobility
from .org.yunlink.telemetry.v1 import telemetry_pb2 as telemetry
from .com.yundrone.sunray.v1 import sunray_pb2 as sunray
from .validation import (
    valid_metric_key,
    validate_summary_snapshot,
    validate_emergency_kill_goal,
    validate_uav_direct_control_goal,
    validate_uav_waypoint_mission_goal,
)

__all__ = [
    "MOBILITY_PROFILE_ID",
    "TELEMETRY_PROFILE_ID",
    "SUNRAY_PROFILE_ID",
    "mobility",
    "telemetry",
    "sunray",
    "valid_metric_key",
    "validate_summary_snapshot",
    "validate_emergency_kill_goal",
    "validate_uav_direct_control_goal",
    "validate_uav_waypoint_mission_goal",
]
