#!/usr/bin/env python3

from __future__ import annotations

import argparse
import time

from yunlink import AgentType, Runtime, RuntimeConfig, TargetSelector, VehicleCoreState


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Python air-side 2 GCS -> 2 UAV helper.")
    parser.add_argument("uav1_udp_bind", type=int)
    parser.add_argument("uav1_udp_target", type=int)
    parser.add_argument("uav1_tcp_listen", type=int)
    parser.add_argument("uav2_udp_bind", type=int)
    parser.add_argument("uav2_udp_target", type=int)
    parser.add_argument("uav2_tcp_listen", type=int)
    return parser.parse_args()


def wait_for_lease(runtime: Runtime, timeout_s: float) -> object:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        lease = runtime.current_authority()
        if lease is not None:
            return lease
        time.sleep(0.02)
    raise RuntimeError("authority was not granted in time")


def wait_for_release(runtime: Runtime, session_id: int, timeout_s: float) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        lease = runtime.current_authority()
        if lease is None or lease.session_id != session_id:
            return
        time.sleep(0.02)
    raise RuntimeError(f"authority release was not observed for session={session_id}")


def main() -> int:
    args = parse_args()
    uav1 = Runtime.start(
        RuntimeConfig(
            args.uav1_udp_bind,
            args.uav1_udp_target,
            args.uav1_tcp_listen,
            AgentType.UAV,
            1,
        )
    )
    uav2 = Runtime.start(
        RuntimeConfig(
            args.uav2_udp_bind,
            args.uav2_udp_target,
            args.uav2_tcp_listen,
            AgentType.UAV,
            2,
        )
    )
    try:
        lease1 = wait_for_lease(uav1, timeout_s=6.0)
        lease2 = wait_for_lease(uav2, timeout_s=6.0)

        uav1.publish_vehicle_core_state(
            lease1.peer,
            TargetSelector.entity(AgentType.GROUND_STATION, 7),
            VehicleCoreState(True, 3, 11.0, 12.0, 13.0, 0.1, 0.2, 0.3, 71.0),
            lease1.session_id,
        )
        uav2.publish_vehicle_core_state(
            lease2.peer,
            TargetSelector.entity(AgentType.GROUND_STATION, 8),
            VehicleCoreState(True, 4, 21.0, 22.0, 23.0, 0.1, 0.2, 0.3, 82.0),
            lease2.session_id,
        )

        wait_for_release(uav1, lease1.session_id, timeout_s=6.0)
        wait_for_release(uav2, lease2.session_id, timeout_s=6.0)
        print("python-air-2gcs-2uav ok")
        return 0
    finally:
        uav2.close()
        uav1.close()


if __name__ == "__main__":
    raise SystemExit(main())
