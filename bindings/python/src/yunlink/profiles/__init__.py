"""Generated optional YunLink Profile messages."""

MOBILITY_PROFILE_ID = "org.yunlink.mobility"
SUNRAY_PROFILE_ID = "com.yundrone.sunray"

from .org.yunlink.mobility.v1 import mobility_pb2 as mobility
from .com.yundrone.sunray.v1 import sunray_pb2 as sunray

__all__ = ["MOBILITY_PROFILE_ID", "SUNRAY_PROFILE_ID", "mobility", "sunray"]
