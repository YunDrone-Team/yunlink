#!/usr/bin/env python3

from __future__ import annotations

import argparse
import queue
import time

from yunlink import (
    AgentType,
    CommandResultEvent,
    ControlSource,
    GotoCommand,
    Runtime,
    RuntimeConfig,
    TargetSelector,
    VehicleCoreStateEvent,
)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Python ground-side roundtrip helper.")
    parser.add_argument("ip")
    parser.add_argument("port", type=int)
    parser.add_argument("udp_bind", type=int)
    parser.add_argument("udp_target", type=int)
    parser.add_argument("tcp_listen", type=int)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    runtime = Runtime.start(
        RuntimeConfig(
            args.udp_bind,
            args.udp_target,
            args.tcp_listen,
            AgentType.GROUND_STATION,
            7,
        )
    )
    try:
        events = runtime.subscribe()
        peer = runtime.connect(args.ip, args.port)
        session = runtime.open_session(peer, "python-ground")
        target = TargetSelector.entity(AgentType.UAV, 1)
        runtime.request_authority(
            peer, session, target, ControlSource.GROUND_STATION, 3000, False
        )
        runtime.renew_authority(
            peer, session, target, ControlSource.GROUND_STATION, 4500
        )
        runtime.publish_goto(peer, session, target, GotoCommand(5.0, 1.0, 3.0, 0.25))

        deadline = time.time() + 4.0
        saw_state = False
        result_count = 0
        while time.time() < deadline:
            try:
                event = events.get(timeout=0.1)
            except queue.Empty:
                continue
            if isinstance(event, CommandResultEvent) and event.session_id == session.session_id:
                result_count += 1
            if (
                isinstance(event, VehicleCoreStateEvent)
                and event.session_id == session.session_id
                and abs(event.battery_percent - 76.5) < 1e-6
            ):
                saw_state = True
            if saw_state and result_count >= 4:
                print("python-ground-roundtrip ok")
                runtime.release_authority(peer, session, target)
                return 0
        raise RuntimeError("did not observe state + command result flow")
    finally:
        runtime.close()


if __name__ == "__main__":
    raise SystemExit(main())
