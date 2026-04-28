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
    parser = argparse.ArgumentParser(
        description="Python ground helper for authority expiry and explicit reacquire."
    )
    parser.add_argument("ip")
    parser.add_argument("port", type=int)
    parser.add_argument("udp_bind", type=int)
    parser.add_argument("udp_target", type=int)
    parser.add_argument("tcp_listen", type=int)
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
        connect_started = time.time()
        peer = runtime.connect(args.ip, args.port)
        connect_ms = (time.time() - connect_started) * 1000.0

        session_started = time.time()
        session = runtime.open_session(peer, "python-ground-expiry")
        session_ready_ms = (time.time() - session_started) * 1000.0
        target = TargetSelector.entity(AgentType.UAV, 1)
        goto = GotoCommand(5.0, 1.0, 3.0, 0.25)

        authority_started = time.time()
        runtime.request_authority(
            peer, session, target, ControlSource.GROUND_STATION, 200, False
        )
        wait_for_state(events, session.session_id, 76.5, timeout_s=4.0)
        state_first_seen_ms = (time.time() - authority_started) * 1000.0

        command_started = time.time()
        runtime.publish_goto(peer, session, target, goto)
        expect_result_count(events, session.session_id, timeout_s=3.0, minimum=4)
        command_result_ms = (time.time() - command_started) * 1000.0

        time.sleep(0.35)
        drain_events(events)
        runtime.publish_goto(peer, session, target, goto)
        expect_failed_result(events, session.session_id, "no-authority", timeout_s=1.5)

        recovery_started = time.time()
        runtime.request_authority(
            peer, session, target, ControlSource.GROUND_STATION, 3000, False
        )
        wait_for_state(events, session.session_id, 76.5, timeout_s=4.0)
        runtime.publish_goto(peer, session, target, goto)
        expect_result_count(events, session.session_id, timeout_s=3.0, minimum=4)
        recovery_ms = (time.time() - recovery_started) * 1000.0

        runtime.release_authority(peer, session, target)
        metrics = {
            "connect_ms": connect_ms,
            "session_ready_ms": session_ready_ms,
            "authority_acquire_ms": state_first_seen_ms,
            "command_result_ms": command_result_ms,
            "state_first_seen_ms": state_first_seen_ms,
            "recovery_ms": recovery_ms,
        }
        print(f"YUNLINK_METRICS {json.dumps(metrics)}")
        print("python-ground-authority-expiry-recovery ok")
        return 0
    finally:
        runtime.close()


if __name__ == "__main__":
    raise SystemExit(main())
