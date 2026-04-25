#!/usr/bin/env python3

from __future__ import annotations

import argparse
import time

from yunlink import AgentType, Runtime, RuntimeConfig, TargetSelector, VehicleCoreState


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Python air-side roundtrip helper.")
    parser.add_argument("udp_bind", type=int)
    parser.add_argument("udp_target", type=int)
    parser.add_argument("tcp_listen", type=int)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    runtime = Runtime.start(
        RuntimeConfig(args.udp_bind, args.udp_target, args.tcp_listen, AgentType.UAV, 1)
    )
    try:
        deadline = time.time() + 4.0
        lease = None
        while time.time() < deadline:
            lease = runtime.current_authority()
            if lease is not None:
                break
            time.sleep(0.02)
        if lease is None:
            raise RuntimeError("authority was not granted in time")

        runtime.publish_vehicle_core_state(
            lease.peer,
            TargetSelector.broadcast(AgentType.GROUND_STATION),
            VehicleCoreState(True, 3, 11.0, 12.0, 13.0, 0.1, 0.2, 0.3, 76.5),
            lease.session_id,
        )
        time.sleep(0.3)
        print("python-air-roundtrip ok")
        return 0
    finally:
        runtime.close()


if __name__ == "__main__":
    raise SystemExit(main())
