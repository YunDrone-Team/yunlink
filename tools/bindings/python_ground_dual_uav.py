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
    parser = argparse.ArgumentParser(description="Python ground-side dual-UAV routing helper.")
    parser.add_argument("ip")
    parser.add_argument("uav1_port", type=int)
    parser.add_argument("uav2_port", type=int)
    parser.add_argument("ground_udp_bind", type=int)
    parser.add_argument("ground_udp_target", type=int)
    parser.add_argument("ground_tcp_listen", type=int)
    return parser.parse_args()


def drain_events(events: queue.Queue) -> list[object]:
    drained: list[object] = []
    while True:
        try:
            drained.append(events.get_nowait())
        except queue.Empty:
            return drained


def wait_for_state(events: queue.Queue, session_id: int, battery_percent: float, timeout_s: float) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            event = events.get(timeout=0.1)
        except queue.Empty:
            continue
        if (
            isinstance(event, VehicleCoreStateEvent)
            and event.session_id == session_id
            and abs(event.battery_percent - battery_percent) < 1e-6
        ):
            return
    raise RuntimeError(
        f"did not observe state battery={battery_percent} for session={session_id}"
    )


def expect_no_state(events: queue.Queue, battery_percent: float, timeout_s: float) -> None:
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            event = events.get(timeout=0.1)
        except queue.Empty:
            return
        if isinstance(event, VehicleCoreStateEvent) and abs(event.battery_percent - battery_percent) < 1e-6:
            raise RuntimeError(f"unexpected state battery={battery_percent} leaked to ground")


def collect_results(events: queue.Queue, session_id: int, timeout_s: float) -> list[CommandResultEvent]:
    deadline = time.time() + timeout_s
    results: list[CommandResultEvent] = []
    while time.time() < deadline:
        try:
            event = events.get(timeout=0.1)
        except queue.Empty:
            continue
        if isinstance(event, CommandResultEvent) and event.session_id == session_id:
            results.append(event)
            if len(results) >= 4:
                break
    return results


def expect_result_count(events: queue.Queue, session_id: int, timeout_s: float, minimum: int) -> None:
    results = collect_results(events, session_id, timeout_s)
    if len(results) < minimum:
        raise RuntimeError(
            f"expected at least {minimum} command results for session={session_id}, got {len(results)}"
        )


def expect_no_results(events: queue.Queue, session_id: int, timeout_s: float) -> None:
    results = collect_results(events, session_id, timeout_s)
    if results:
        raise RuntimeError(f"unexpected command results for session={session_id}: {len(results)}")


def main() -> int:
    args = parse_args()
    ground = Runtime.start(
        RuntimeConfig(
            args.ground_udp_bind,
            args.ground_udp_target,
            args.ground_tcp_listen,
            AgentType.GROUND_STATION,
            7,
        )
    )
    try:
        events = ground.subscribe()
        peer1 = ground.connect(args.ip, args.uav1_port)
        peer2 = ground.connect(args.ip, args.uav2_port)
        session1 = ground.open_session(peer1, "python-ground-uav1")
        session2 = ground.open_session(peer2, "python-ground-uav2")
        target1 = TargetSelector.entity(AgentType.UAV, 1)
        target2 = TargetSelector.entity(AgentType.UAV, 2)
        goto = GotoCommand(5.0, 1.0, 3.0, 0.25)

        ground.request_authority(
            peer1, session1, target1, ControlSource.GROUND_STATION, 3000, False
        )
        ground.request_authority(
            peer2, session2, target2, ControlSource.GROUND_STATION, 3000, False
        )
        ground.renew_authority(
            peer1, session1, target1, ControlSource.GROUND_STATION, 4500
        )
        ground.renew_authority(
            peer2, session2, target2, ControlSource.GROUND_STATION, 4500
        )

        wait_for_state(events, session1.session_id, 71.0, timeout_s=4.0)
        expect_no_state(events, 91.0, timeout_s=0.4)
        wait_for_state(events, session2.session_id, 82.0, timeout_s=4.0)

        drain_events(events)
        ground.publish_goto(peer1, session1, target2, goto)
        expect_no_results(events, session1.session_id, timeout_s=0.5)

        ground.publish_goto(peer1, session1, target1, goto)
        expect_result_count(events, session1.session_id, timeout_s=3.0, minimum=4)

        ground.publish_goto(peer2, session2, target2, goto)
        expect_result_count(events, session2.session_id, timeout_s=3.0, minimum=4)

        ground.release_authority(peer2, session2, target2)
        ground.release_authority(peer1, session1, target1)
        print("python-ground-dual-uav ok")
        return 0
    finally:
        ground.close()


if __name__ == "__main__":
    raise SystemExit(main())
