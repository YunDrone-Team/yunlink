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
    parser = argparse.ArgumentParser(description="Python ground-side authority competition helper.")
    parser.add_argument("ip")
    parser.add_argument("port", type=int)
    parser.add_argument("ground_a_udp_bind", type=int)
    parser.add_argument("ground_a_udp_target", type=int)
    parser.add_argument("ground_a_tcp_listen", type=int)
    parser.add_argument("ground_b_udp_bind", type=int)
    parser.add_argument("ground_b_udp_target", type=int)
    parser.add_argument("ground_b_tcp_listen", type=int)
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


def expect_failed_result(
    events: queue.Queue, session_id: int, detail: str, timeout_s: float
) -> None:
    results = collect_results(events, session_id, timeout_s)
    if not any(result.phase == 5 and result.detail == detail for result in results):
        raise RuntimeError(
            f"expected failed command result detail={detail!r} "
            f"for session={session_id}, got {results!r}"
        )


def main() -> int:
    args = parse_args()
    ground_a = Runtime.start(
        RuntimeConfig(
            args.ground_a_udp_bind,
            args.ground_a_udp_target,
            args.ground_a_tcp_listen,
            AgentType.GROUND_STATION,
            7,
        )
    )
    ground_b = Runtime.start(
        RuntimeConfig(
            args.ground_b_udp_bind,
            args.ground_b_udp_target,
            args.ground_b_tcp_listen,
            AgentType.GROUND_STATION,
            8,
        )
    )
    try:
        events_a = ground_a.subscribe()
        events_b = ground_b.subscribe()

        peer_a = ground_a.connect(args.ip, args.port)
        peer_b = ground_b.connect(args.ip, args.port)
        session_a = ground_a.open_session(peer_a, "python-ground-a")
        _unused_session_b = ground_b.open_session(peer_b, "python-ground-b-unused")
        session_b = ground_b.open_session(peer_b, "python-ground-b")
        target = TargetSelector.entity(AgentType.UAV, 1)
        goto = GotoCommand(5.0, 1.0, 3.0, 0.25)

        ground_a.request_authority(
            peer_a, session_a, target, ControlSource.GROUND_STATION, 3000, False
        )
        ground_a.renew_authority(
            peer_a, session_a, target, ControlSource.GROUND_STATION, 4500
        )
        wait_for_state(events_a, session_a.session_id, 71.0, timeout_s=4.0)
        ground_a.publish_goto(peer_a, session_a, target, goto)
        expect_result_count(events_a, session_a.session_id, timeout_s=3.0, minimum=4)

        drain_events(events_b)
        ground_b.request_authority(
            peer_b, session_b, target, ControlSource.GROUND_STATION, 3000, False
        )
        ground_b.publish_goto(peer_b, session_b, target, goto)
        expect_failed_result(events_b, session_b.session_id, "no-authority", timeout_s=1.0)

        ground_b.request_authority(
            peer_b, session_b, target, ControlSource.GROUND_STATION, 3000, True
        )
        ground_b.renew_authority(
            peer_b, session_b, target, ControlSource.GROUND_STATION, 4500
        )
        wait_for_state(events_b, session_b.session_id, 72.0, timeout_s=4.0)
        ground_b.publish_goto(peer_b, session_b, target, goto)
        expect_result_count(events_b, session_b.session_id, timeout_s=3.0, minimum=4)

        drain_events(events_a)
        ground_a.publish_goto(peer_a, session_a, target, goto)
        expect_failed_result(events_a, session_a.session_id, "no-authority", timeout_s=1.0)

        ground_b.release_authority(peer_b, session_b, target)
        time.sleep(0.2)

        ground_a.request_authority(
            peer_a, session_a, target, ControlSource.GROUND_STATION, 3000, False
        )
        ground_a.renew_authority(
            peer_a, session_a, target, ControlSource.GROUND_STATION, 4500
        )
        wait_for_state(events_a, session_a.session_id, 73.0, timeout_s=4.0)
        ground_a.publish_goto(peer_a, session_a, target, goto)
        expect_result_count(events_a, session_a.session_id, timeout_s=3.0, minimum=4)

        ground_a.release_authority(peer_a, session_a, target)
        print("python-ground-competition ok")
        return 0
    finally:
        ground_b.close()
        ground_a.close()


if __name__ == "__main__":
    raise SystemExit(main())
