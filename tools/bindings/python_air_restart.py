#!/usr/bin/env python3

from __future__ import annotations

import argparse
import time

from yunlink import AgentType, Runtime, RuntimeConfig, TargetSelector, VehicleCoreState


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Python air-side restart helper.")
    parser.add_argument("udp_bind", type=int)
    parser.add_argument("udp_target", type=int)
    parser.add_argument("tcp_listen", type=int)
    parser.add_argument("--rounds", type=int, default=2)
    return parser.parse_args()


def wait_for_new_lease(runtime: Runtime) -> object:
    deadline = time.time() + 4.0
    while time.time() < deadline:
        lease = runtime.current_authority()
        if lease is not None:
            return lease
        time.sleep(0.02)
    raise RuntimeError("authority was not granted in time")


def wait_for_release(runtime: Runtime, session_id: int) -> None:
    deadline = time.time() + 4.0
    while time.time() < deadline:
        lease = runtime.current_authority()
        if lease is None or lease.session_id != session_id:
            return
        time.sleep(0.02)
    raise RuntimeError("authority was not released in time")


def run_round(args: argparse.Namespace) -> None:
    print("python-air-restart: start runtime", flush=True)
    runtime = Runtime.start(
        RuntimeConfig(args.udp_bind, args.udp_target, args.tcp_listen, AgentType.UAV, 1)
    )
    try:
        print("python-air-restart: wait_for_new_lease", flush=True)
        lease = wait_for_new_lease(runtime)
        print("python-air-restart: publish_vehicle_core_state", flush=True)
        runtime.publish_vehicle_core_state(
            lease.peer,
            TargetSelector.broadcast(AgentType.GROUND_STATION),
            VehicleCoreState(True, 3, 11.0, 12.0, 13.0, 0.1, 0.2, 0.3, 76.5),
            lease.session_id,
        )
        print("python-air-restart: wait_for_release", flush=True)
        wait_for_release(runtime, lease.session_id)
    finally:
        print("python-air-restart: close runtime", flush=True)
        runtime.close()


def main() -> int:
    args = parse_args()
    for round_index in range(args.rounds):
        run_round(args)
        if round_index + 1 < args.rounds:
            time.sleep(0.1)
    print("python-air-restart ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
