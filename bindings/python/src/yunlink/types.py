from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
from typing import Any


class AgentType(IntEnum):
    GROUND_STATION = 1
    UAV = 2


class ControlSource(IntEnum):
    GROUND_STATION = 1


@dataclass(frozen=True)
class RuntimeConfig:
    udp_bind_port: int
    udp_target_port: int
    tcp_listen_port: int
    agent_type: AgentType
    agent_id: int
    shared_secret: str = "yunlink-secret"
    multicast_group: str = "224.1.1.1"

    def to_native(self) -> dict[str, int | str]:
        role = 2 if self.agent_type == AgentType.GROUND_STATION else 3
        return {
            "udp_bind_port": self.udp_bind_port,
            "udp_target_port": self.udp_target_port,
            "tcp_listen_port": self.tcp_listen_port,
            "agent_type": int(self.agent_type),
            "agent_id": self.agent_id,
            "role": role,
            "shared_secret": self.shared_secret,
            "multicast_group": self.multicast_group,
        }


@dataclass(frozen=True)
class PeerConnection:
    id: str


@dataclass(frozen=True)
class Session:
    session_id: int


@dataclass(frozen=True)
class TargetSelector:
    scope: int
    target_type: int
    entity_id: int
    group_id: int = 0

    @staticmethod
    def entity(agent_type: AgentType, entity_id: int) -> "TargetSelector":
        return TargetSelector(1, int(agent_type), entity_id, 0)

    @staticmethod
    def broadcast(agent_type: AgentType) -> "TargetSelector":
        return TargetSelector(3, int(agent_type), 0, 0)

    def to_native(self) -> dict[str, int]:
        return {
            "scope": self.scope,
            "target_type": self.target_type,
            "entity_id": self.entity_id,
            "group_id": self.group_id,
        }


@dataclass(frozen=True)
class GotoCommand:
    x_m: float
    y_m: float
    z_m: float
    yaw_rad: float

    def to_native(self) -> dict[str, float]:
        return {
            "x_m": self.x_m,
            "y_m": self.y_m,
            "z_m": self.z_m,
            "yaw_rad": self.yaw_rad,
        }


@dataclass(frozen=True)
class VehicleCoreState:
    armed: bool
    nav_mode: int
    x_m: float
    y_m: float
    z_m: float
    vx_mps: float
    vy_mps: float
    vz_mps: float
    battery_percent: float

    def to_native(self) -> dict[str, Any]:
        return {
            "armed": self.armed,
            "nav_mode": self.nav_mode,
            "x_m": self.x_m,
            "y_m": self.y_m,
            "z_m": self.z_m,
            "vx_mps": self.vx_mps,
            "vy_mps": self.vy_mps,
            "vz_mps": self.vz_mps,
            "battery_percent": self.battery_percent,
        }


@dataclass(frozen=True)
class CommandHandle:
    session_id: int
    message_id: int
    correlation_id: int


@dataclass(frozen=True)
class AuthorityLease:
    state: int
    session_id: int
    peer: PeerConnection
