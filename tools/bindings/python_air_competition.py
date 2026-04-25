#!/usr/bin/env python3

from __future__ import annotations

import argparse
import time

from yunlink import AgentType, Runtime, RuntimeConfig, TargetSelector, VehicleCoreState


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Python air-side authority competition helper.")
    parser.add_argument("udp_bind", type=int)
    parser.add_argument("udp_target", type=int)
    parser.add_argument("tcp_listen", type=int)
    parser.add_argument("--expected-authority-switches", type=int, default=3)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    runtime = Runtime.start(
        RuntimeConfig(args.udp_bind, args.udp_target, args.tcp_listen, AgentType.UAV, 1)
    )
    try:
        authority_switches = 0
        release_count = 0
        deadline = time.time() + 12.0
        last_session_id: int | None = None

        while time.time() < deadline:
            lease = runtime.current_authority()
            if lease is None:
                if last_session_id is not None:
                    release_count += 1
                    last_session_id = None
                if authority_switches >= args.expected_authority_switches and release_count >= 1:
                    print("python-air-competition ok")
                    return 0
                time.sleep(0.02)
                continue

            if lease.session_id != last_session_id:
                last_session_id = lease.session_id
                authority_switches += 1
                runtime.publish_vehicle_core_state(
                    lease.peer,
                    TargetSelector.broadcast(AgentType.GROUND_STATION),
                    VehicleCoreState(
                        True,
                        3,
                        10.0 + authority_switches,
                        11.0 + authority_switches,
                        12.0 + authority_switches,
                        0.1,
                        0.2,
                        0.3,
                        70.0 + authority_switches,
                    ),
                    lease.session_id,
                )
            time.sleep(0.02)

        raise RuntimeError(
            f"competition authority sequence incomplete: switches={authority_switches} "
            f"release_count={release_count}"
        )
    finally:
        runtime.close()


if __name__ == "__main__":
    raise SystemExit(main())
