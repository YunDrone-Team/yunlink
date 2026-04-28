#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
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
    parser = argparse.ArgumentParser(description="Python ground-side 2 GCS -> 2 UAV helper.")
    parser.add_argument("ip")
    parser.add_argument("uav1_port", type=int)
    parser.add_argument("uav2_port", type=int)
    parser.add_argument("ground_a_udp_bind", type=int)
    parser.add_argument("ground_a_udp_target", type=int)
    parser.add_argument("ground_a_tcp_listen", type=int)
    parser.add_argument("ground_b_udp_bind", type=int)
    parser.add_argument("ground_b_udp_target", type=int)
    parser.add_argument("ground_b_tcp_listen", type=int)
    return parser.parse_args()


def drain_events(events: queue.Queue) -> None:
    while True:
        try:
            events.get_nowait()
        except queue.Empty:
            return


def wait_for_state(
    events: queue.Queue, session_id: int, battery_percent: float, timeout_s: float
) -> None:
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
        if (
            isinstance(event, VehicleCoreStateEvent)
            and abs(event.battery_percent - battery_percent) < 1e-6
        ):
            raise RuntimeError(f"unexpected state battery={battery_percent} leaked")


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
    deadline = time.time() + timeout_s
    while time.time() < deadline:
        try:
            event = events.get(timeout=0.1)
        except queue.Empty:
            continue
        if (
            isinstance(event, CommandResultEvent)
            and event.session_id == session_id
            and event.phase == 5
            and event.detail == detail
        ):
            return
    raise RuntimeError(
        f"expected failed command result detail={detail!r} for session={session_id}"
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

        connect_started = time.time()
        peer_a1 = ground_a.connect(args.ip, args.uav1_port)
        peer_a2 = ground_a.connect(args.ip, args.uav2_port)
        peer_b1 = ground_b.connect(args.ip, args.uav1_port)
        peer_b2 = ground_b.connect(args.ip, args.uav2_port)
        connect_ms = (time.time() - connect_started) * 1000.0

        session_started = time.time()
        session_a1 = ground_a.open_session(peer_a1, "python-ground-a-uav1")
        session_a2 = ground_a.open_session(peer_a2, "python-ground-a-uav2")
        session_b1 = ground_b.open_session(peer_b1, "python-ground-b-uav1")
        session_b2 = ground_b.open_session(peer_b2, "python-ground-b-uav2")
        session_ready_ms = (time.time() - session_started) * 1000.0

        target1 = TargetSelector.entity(AgentType.UAV, 1)
        target2 = TargetSelector.entity(AgentType.UAV, 2)
        goto = GotoCommand(5.0, 1.0, 3.0, 0.25)

        authority_started = time.time()
        ground_a.request_authority(
            peer_a1, session_a1, target1, ControlSource.GROUND_STATION, 3000, False
        )
        ground_b.request_authority(
            peer_b2, session_b2, target2, ControlSource.GROUND_STATION, 3000, False
        )
        state_started = time.time()
        wait_for_state(events_a, session_a1.session_id, 71.0, timeout_s=4.0)
        expect_no_state(events_a, 82.0, timeout_s=0.4)
        wait_for_state(events_b, session_b2.session_id, 82.0, timeout_s=4.0)
        expect_no_state(events_b, 71.0, timeout_s=0.4)
        state_first_seen_ms = (time.time() - state_started) * 1000.0
        authority_acquire_ms = (time.time() - authority_started) * 1000.0

        drain_events(events_a)
        drain_events(events_b)
        ground_a.publish_goto(peer_a2, session_a2, target2, goto)
        expect_failed_result(events_a, session_a2.session_id, "no-authority", timeout_s=1.5)
        ground_b.publish_goto(peer_b1, session_b1, target1, goto)
        expect_failed_result(events_b, session_b1.session_id, "no-authority", timeout_s=1.5)

        command_started = time.time()
        ground_a.publish_goto(peer_a1, session_a1, target1, goto)
        expect_result_count(events_a, session_a1.session_id, timeout_s=3.0, minimum=4)
        ground_b.publish_goto(peer_b2, session_b2, target2, goto)
        expect_result_count(events_b, session_b2.session_id, timeout_s=3.0, minimum=4)
        command_result_ms = (time.time() - command_started) * 1000.0

        ground_b.release_authority(peer_b2, session_b2, target2)
        ground_a.release_authority(peer_a1, session_a1, target1)
        metrics = {
            "connect_ms": connect_ms,
            "session_ready_ms": session_ready_ms,
            "authority_acquire_ms": authority_acquire_ms,
            "command_result_ms": command_result_ms,
            "state_first_seen_ms": state_first_seen_ms,
            "recovery_ms": 0.0,
        }
        print(f"YUNLINK_METRICS {json.dumps(metrics)}")
        print("python-ground-2gcs-2uav ok")
        return 0
    finally:
        ground_b.close()
        ground_a.close()


if __name__ == "__main__":
    raise SystemExit(main())
