"""Generated optional YunLink Profile messages."""

MOBILITY_PROFILE_ID = "org.yunlink.mobility"
TELEMETRY_PROFILE_ID = "org.yunlink.telemetry"
MEDIA_PROFILE_ID = "org.yunlink.media"
SYSTEM_PROFILE_ID = "org.yunlink.system"
SHELL_PROFILE_ID = "org.yunlink.shell"
SUNRAY_PROFILE_ID = "com.yundrone.sunray"

from .org.yunlink.mobility.v1 import mobility_pb2 as mobility
from .org.yunlink.telemetry.v1 import telemetry_pb2 as telemetry
from .org.yunlink.media.v1 import media_pb2 as media
from .org.yunlink.system.v1 import system_pb2 as system
from .org.yunlink.shell.v1 import shell_pb2 as shell
from .com.yundrone.sunray.v2 import sunray_pb2 as sunray
from .validation import (
    valid_metric_key,
    validate_flight_control_state,
    validate_summary_snapshot,
    validate_camera_catalog_snapshot,
    validate_camera_descriptor,
    validate_media_asset_chunk,
    validate_media_asset_ref,
    validate_media_asset_item,
    validate_media_asset_list_request,
    validate_media_asset_list_response,
    validate_camera_start_rtsp_response,
    validate_clock_sync_request,
    validate_clock_sync_response,
    validate_emergency_kill_goal,
    validate_formation_leader_target_request,
    validate_formation_set_request,
    validate_formation_state,
    validate_land_goal,
    validate_planner_set_home_request,
    validate_shell_close_request,
    validate_shell_open_request,
    validate_shell_resize_request,
    validate_shell_write_request,
    validate_takeoff_goal,
    validate_gimbal_angle_goal,
    validate_gimbal_rate_goal,
    validate_gimbal_zoom_absolute_goal,
    validate_uav_direct_control_goal,
    validate_uav_nav_goal,
    validate_uav_waypoint_mission_goal,
    validate_ugv_move_point_goal,
    validate_ugv_control_state,
    validate_ugv_nav_goal,
    validate_ugv_planning_state,
    validate_ugv_velocity_goal,
    validate_ugv_waypoint_mission_goal,
)

__all__ = [
    "MOBILITY_PROFILE_ID",
    "TELEMETRY_PROFILE_ID",
    "MEDIA_PROFILE_ID",
    "SYSTEM_PROFILE_ID",
    "SHELL_PROFILE_ID",
    "SUNRAY_PROFILE_ID",
    "mobility",
    "telemetry",
    "media",
    "system",
    "shell",
    "sunray",
    "valid_metric_key",
    "validate_flight_control_state",
    "validate_summary_snapshot",
    "validate_camera_catalog_snapshot",
    "validate_camera_descriptor",
    "validate_media_asset_chunk",
    "validate_media_asset_ref",
    "validate_media_asset_item",
    "validate_media_asset_list_request",
    "validate_media_asset_list_response",
    "validate_camera_start_rtsp_response",
    "validate_clock_sync_request",
    "validate_clock_sync_response",
    "validate_emergency_kill_goal",
    "validate_formation_leader_target_request",
    "validate_formation_set_request",
    "validate_formation_state",
    "validate_land_goal",
    "validate_planner_set_home_request",
    "validate_shell_close_request",
    "validate_shell_open_request",
    "validate_shell_resize_request",
    "validate_shell_write_request",
    "validate_takeoff_goal",
    "validate_gimbal_angle_goal",
    "validate_gimbal_rate_goal",
    "validate_gimbal_zoom_absolute_goal",
    "validate_uav_direct_control_goal",
    "validate_uav_nav_goal",
    "validate_uav_waypoint_mission_goal",
    "validate_ugv_move_point_goal",
    "validate_ugv_control_state",
    "validate_ugv_nav_goal",
    "validate_ugv_planning_state",
    "validate_ugv_velocity_goal",
    "validate_ugv_waypoint_mission_goal",
]
