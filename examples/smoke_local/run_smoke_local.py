#!/usr/bin/env python3
"""
@file examples/smoke_local/run_smoke_local.py
@brief yunlink 本机多进程冒烟测试编排脚本。
"""

import argparse
import os
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import List, Optional


@dataclass
class ProcResult:
    name: str
    returncode: Optional[int]
    stdout: str
    stderr: str


class ManagedProc:
    def __init__(self, name: str, cmd: List[str], cwd: Path) -> None:
        self.name = name
        self.cmd = cmd
        self.cwd = cwd
        self.p: Optional[subprocess.Popen[str]] = None

    def start(self) -> None:
        self.p = subprocess.Popen(
            self.cmd,
            cwd=self.cwd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid if os.name != "nt" else None,
        )

    def terminate(self) -> None:
        if not self.p or self.p.poll() is not None:
            return
        try:
            if os.name == "nt":
                self.p.terminate()
            else:
                os.killpg(os.getpgid(self.p.pid), signal.SIGTERM)
        except ProcessLookupError:
            pass

    def kill(self) -> None:
        if not self.p or self.p.poll() is not None:
            return
        try:
            if os.name == "nt":
                self.p.kill()
            else:
                os.killpg(os.getpgid(self.p.pid), signal.SIGKILL)
        except ProcessLookupError:
            pass

    def wait(self, timeout: float) -> ProcResult:
        if not self.p:
            raise RuntimeError(f"{self.name} not started")
        try:
            out, err = self.p.communicate(timeout=timeout)
            return ProcResult(self.name, self.p.returncode, out, err)
        except subprocess.TimeoutExpired:
            return ProcResult(self.name, None, "", f"timeout after {timeout}s")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run local multi-process smoke tests for examples."
    )
    parser.add_argument(
        "--bin-dir", required=True, help="Build output directory containing example binaries."
    )
    return parser.parse_args()


def exe(bin_dir: Path, name: str) -> str:
    suffix = ".exe" if os.name == "nt" else ""
    path = bin_dir / f"{name}{suffix}"
    if not path.exists():
        raise FileNotFoundError(f"missing executable: {path}")
    return str(path)


def run_checked(cmd: List[str], cwd: Path, timeout: float, name: str) -> ProcResult:
    p = subprocess.Popen(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        out, err = p.communicate(timeout=timeout)
        return ProcResult(name, p.returncode, out, err)
    except subprocess.TimeoutExpired:
        p.kill()
        out, err = p.communicate()
        return ProcResult(name, None, out, err + f"\n{name}: timeout after {timeout}s")


def print_result(result: ProcResult) -> None:
    print(f"[{result.name}] returncode={result.returncode}")
    if result.stdout.strip():
        print(f"[{result.name}] stdout:\n{result.stdout.strip()}")
    if result.stderr.strip():
        print(f"[{result.name}] stderr:\n{result.stderr.strip()}")


def require_ok(result: ProcResult) -> None:
    print_result(result)
    if result.returncode != 0:
        raise RuntimeError(f"{result.name} failed")


def wait_for_tcp_listener(host: str, port: int, timeout_s: float) -> bool:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def scenario_tcp_direct(bin_dir: Path, cwd: Path) -> None:
    print("== Scenario 1: TCP direct ==")
    tcp_listen = 21096
    receiver_udp_bind = 21097
    sender_udp_bind = 21197
    sender_udp_target = 21198

    receiver = ManagedProc(
        "telemetry_receiver_tcp",
        [
            exe(bin_dir, "example_telemetry_receiver"),
            "--udp-bind",
            str(receiver_udp_bind),
            "--tcp-listen",
            str(tcp_listen),
            "--timeout-ms",
            "2500",
            "--required-frames",
            "1",
        ],
        cwd,
    )
    receiver.start()
    if not wait_for_tcp_listener("127.0.0.1", tcp_listen, timeout_s=2.5):
        raise RuntimeError("telemetry_receiver_tcp did not open its TCP listener in time")

    sender = run_checked(
        [
            exe(bin_dir, "example_tcp_command_client"),
            "127.0.0.1",
            str(tcp_listen),
            "--udp-bind",
            str(sender_udp_bind),
            "--udp-target",
            str(sender_udp_target),
            "--tcp-listen",
            "21099",
        ],
        cwd,
        timeout=4.0,
        name="tcp_command_client",
    )
    try:
        require_ok(sender)
        recv = receiver.wait(timeout=4.0)
        require_ok(recv)
    finally:
        receiver.terminate()
        time.sleep(0.1)
        receiver.kill()


def scenario_udp_bridge(bin_dir: Path, cwd: Path) -> None:
    print("== Scenario 2: UDP -> TCP bridge ==")
    sink_udp_bind = 22097
    sink_tcp_listen = 22096
    bridge_udp_bind = 22098
    bridge_tcp_listen = 22099

    sink = ManagedProc(
        "telemetry_receiver_sink",
        [
            exe(bin_dir, "example_telemetry_receiver"),
            "--udp-bind",
            str(sink_udp_bind),
            "--tcp-listen",
            str(sink_tcp_listen),
            "--timeout-ms",
            "5000",
            "--required-frames",
            "1",
        ],
        cwd,
    )
    bridge = ManagedProc(
        "udp_tcp_bridge",
        [
            exe(bin_dir, "example_udp_tcp_bridge"),
            "127.0.0.1",
            str(sink_tcp_listen),
            "--udp-bind",
            str(bridge_udp_bind),
            "--tcp-listen",
            str(bridge_tcp_listen),
            "--duration-ms",
            "5500",
        ],
        cwd,
    )

    sink.start()
    if not wait_for_tcp_listener("127.0.0.1", sink_tcp_listen, timeout_s=2.5):
        raise RuntimeError("telemetry_receiver_sink did not open its TCP listener in time")

    bridge.start()
    if not wait_for_tcp_listener("127.0.0.1", bridge_tcp_listen, timeout_s=2.5):
        raise RuntimeError("udp_tcp_bridge did not finish startup in time")

    try:
        for attempt in range(1, 4):
            discovery = run_checked(
                [
                    exe(bin_dir, "example_discovery_udp"),
                    "--udp-bind",
                    str(22100 + attempt),
                    "--udp-target",
                    str(bridge_udp_bind),
                    "--udp-target-ip",
                    "127.0.0.1",
                    "--hold-ms",
                    "500",
                ],
                cwd,
                timeout=3.0,
                name=f"discovery_udp_try{attempt}",
            )
            require_ok(discovery)
            time.sleep(0.2)
            if sink.p and sink.p.poll() == 0:
                break

        sink_result = sink.wait(timeout=5.0)
        require_ok(sink_result)
        bridge_result = bridge.wait(timeout=6.0)
        require_ok(bridge_result)
    finally:
        sink.terminate()
        bridge.terminate()
        time.sleep(0.1)
        sink.kill()
        bridge.kill()


def main() -> int:
    args = parse_args()
    bin_dir = Path(args.bin_dir).resolve()
    cwd = bin_dir
    try:
        scenario_tcp_direct(bin_dir, cwd)
        scenario_udp_bridge(bin_dir, cwd)
    except Exception as exc:
        print(f"[smoke] FAIL: {exc}", file=sys.stderr)
        return 1
    print("[smoke] PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
