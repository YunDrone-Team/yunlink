#!/usr/bin/env python3

from __future__ import annotations

import argparse
import os
import signal
import socket
import subprocess
import sys
import time
from dataclasses import dataclass
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
BUILD_DIR = ROOT_DIR / "build" / "ninja-debug"
RUST_EXAMPLE_DIR = ROOT_DIR / "bindings" / "rust" / "target" / "debug" / "examples"
PYTHON_BIN = ROOT_DIR / ".venv" / "bin" / "python"
RUST_BUILD_DIR = next(
    (ROOT_DIR / path)
    for path in sorted(
        Path("bindings/rust/target/debug/build").glob("yunlink-sys-*/out/cmake-build")
    )
)


@dataclass
class ProcResult:
    name: str
    returncode: int | None
    stdout: str
    stderr: str


class ManagedProc:
    def __init__(self, name: str, cmd: list[str], cwd: Path, env: dict[str, str] | None = None) -> None:
        self.name = name
        self.cmd = cmd
        self.cwd = cwd
        self.env = env
        self.proc: subprocess.Popen[str] | None = None

    def start(self) -> None:
        self.proc = subprocess.Popen(
            self.cmd,
            cwd=self.cwd,
            env=self.env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            preexec_fn=os.setsid if os.name != "nt" else None,
        )

    def terminate(self) -> None:
        if not self.proc or self.proc.poll() is not None:
            return
        if os.name == "nt":
            self.proc.terminate()
        else:
            os.killpg(os.getpgid(self.proc.pid), signal.SIGTERM)

    def kill(self) -> None:
        if not self.proc or self.proc.poll() is not None:
            return
        if os.name == "nt":
            self.proc.kill()
        else:
            os.killpg(os.getpgid(self.proc.pid), signal.SIGKILL)

    def wait(self, timeout: float) -> ProcResult:
        assert self.proc is not None
        try:
            stdout, stderr = self.proc.communicate(timeout=timeout)
            return ProcResult(self.name, self.proc.returncode, stdout, stderr)
        except subprocess.TimeoutExpired:
            return ProcResult(self.name, None, "", f"timeout after {timeout}s")


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


def wait_for_tcp_listener(host: str, port: int, timeout_s: float) -> None:
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.2):
                return
        except OSError:
            time.sleep(0.05)
    raise RuntimeError(f"listener {host}:{port} not ready in time")


def cxx_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(BUILD_DIR / "example_cxx_air_roundtrip"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def rust_ground_cmd(ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "ground_roundtrip"),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def rust_ground_recovery_cmd(
    ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int, rounds: int = 2
) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "ground_recovery"),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        str(rounds),
    ]


def rust_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(RUST_EXAMPLE_DIR / "air_roundtrip"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_ground_cmd(ip: str, port: int, udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_ground_roundtrip.py"),
        ip,
        str(port),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_air_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_air_roundtrip.py"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_air_recovery_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_air_recovery.py"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        "--rounds",
        "2",
    ]


def python_air_restart_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_air_restart.py"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
        "--rounds",
        "2",
    ]


def python_air_competition_cmd(udp_bind: int, udp_target: int, tcp_listen: int) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_air_competition.py"),
        str(udp_bind),
        str(udp_target),
        str(tcp_listen),
    ]


def python_ground_competition_cmd(
    ip: str,
    port: int,
    ground_a_udp_bind: int,
    ground_a_udp_target: int,
    ground_a_tcp_listen: int,
    ground_b_udp_bind: int,
    ground_b_udp_target: int,
    ground_b_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_ground_competition.py"),
        ip,
        str(port),
        str(ground_a_udp_bind),
        str(ground_a_udp_target),
        str(ground_a_tcp_listen),
        str(ground_b_udp_bind),
        str(ground_b_udp_target),
        str(ground_b_tcp_listen),
    ]


def python_air_dual_uav_cmd(
    uav1_udp_bind: int,
    uav1_udp_target: int,
    uav1_tcp_listen: int,
    uav2_udp_bind: int,
    uav2_udp_target: int,
    uav2_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_air_dual_uav.py"),
        str(uav1_udp_bind),
        str(uav1_udp_target),
        str(uav1_tcp_listen),
        str(uav2_udp_bind),
        str(uav2_udp_target),
        str(uav2_tcp_listen),
    ]


def python_ground_dual_uav_cmd(
    ip: str,
    uav1_port: int,
    uav2_port: int,
    ground_udp_bind: int,
    ground_udp_target: int,
    ground_tcp_listen: int,
) -> list[str]:
    return [
        str(PYTHON_BIN),
        str(ROOT_DIR / "tools" / "bindings" / "python_ground_dual_uav.py"),
        ip,
        str(uav1_port),
        str(uav2_port),
        str(ground_udp_bind),
        str(ground_udp_target),
        str(ground_tcp_listen),
    ]


def run_pair(name: str, air_cmd: list[str], ground_cmd: list[str], air_port: int) -> None:
    print(f"== {name} ==")
    env = os.environ.copy()
    rust_lib_dir = str(RUST_BUILD_DIR)
    env["DYLD_LIBRARY_PATH"] = (
        rust_lib_dir + os.pathsep + env["DYLD_LIBRARY_PATH"]
        if env.get("DYLD_LIBRARY_PATH")
        else rust_lib_dir
    )
    env["LD_LIBRARY_PATH"] = (
        rust_lib_dir + os.pathsep + env["LD_LIBRARY_PATH"]
        if env.get("LD_LIBRARY_PATH")
        else rust_lib_dir
    )
    env["PATH"] = rust_lib_dir + os.pathsep + env["PATH"]

    air = ManagedProc(f"{name}-air", air_cmd, ROOT_DIR, env)
    air.start()
    try:
        wait_for_tcp_listener("127.0.0.1", air_port, timeout_s=2.5)
        ground = subprocess.run(
            ground_cmd,
            cwd=ROOT_DIR,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=10.0,
        )
        require_ok(ProcResult(f"{name}-ground", ground.returncode, ground.stdout, ground.stderr))
        require_ok(air.wait(timeout=6.0))
    finally:
        air.terminate()
        time.sleep(0.1)
        air.kill()


def run_recovery_pair(
    name: str,
    air_cmd: list[str],
    first_ground_cmd: list[str],
    second_ground_cmd: list[str],
    air_port: int,
) -> None:
    print(f"== {name} ==")
    env = os.environ.copy()
    rust_lib_dir = str(RUST_BUILD_DIR)
    env["DYLD_LIBRARY_PATH"] = (
        rust_lib_dir + os.pathsep + env["DYLD_LIBRARY_PATH"]
        if env.get("DYLD_LIBRARY_PATH")
        else rust_lib_dir
    )
    env["LD_LIBRARY_PATH"] = (
        rust_lib_dir + os.pathsep + env["LD_LIBRARY_PATH"]
        if env.get("LD_LIBRARY_PATH")
        else rust_lib_dir
    )
    env["PATH"] = rust_lib_dir + os.pathsep + env["PATH"]

    air = ManagedProc(f"{name}-air", air_cmd, ROOT_DIR, env)
    air.start()
    try:
        wait_for_tcp_listener("127.0.0.1", air_port, timeout_s=2.5)
        first_ground = subprocess.run(
            first_ground_cmd,
            cwd=ROOT_DIR,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=10.0,
        )
        require_ok(
            ProcResult(
                f"{name}-ground-1",
                first_ground.returncode,
                first_ground.stdout,
                first_ground.stderr,
            )
        )
        second_ground = subprocess.run(
            second_ground_cmd,
            cwd=ROOT_DIR,
            env=env,
            text=True,
            capture_output=True,
            check=False,
            timeout=10.0,
        )
        require_ok(
            ProcResult(
                f"{name}-ground-2",
                second_ground.returncode,
                second_ground.stdout,
                second_ground.stderr,
            )
        )
        require_ok(air.wait(timeout=8.0))
    finally:
        air.terminate()
        time.sleep(0.1)
        air.kill()


def main() -> int:
    parser = argparse.ArgumentParser(description="Run the bindings interop matrix.")
    parser.parse_args()

    run_pair(
        "rust-ground-cxx-air",
        cxx_air_cmd(16030, 16030, 16130),
        rust_ground_cmd("127.0.0.1", 16130, 16031, 16031, 16131),
        16130,
    )
    run_pair(
        "python-ground-cxx-air",
        cxx_air_cmd(16040, 16040, 16140),
        python_ground_cmd("127.0.0.1", 16140, 16041, 16041, 16141),
        16140,
    )
    run_pair(
        "rust-ground-python-air",
        python_air_cmd(16050, 16050, 16150),
        rust_ground_cmd("127.0.0.1", 16150, 16051, 16051, 16151),
        16150,
    )
    run_pair(
        "python-ground-rust-air",
        rust_air_cmd(16060, 16060, 16160),
        python_ground_cmd("127.0.0.1", 16160, 16061, 16061, 16161),
        16160,
    )
    run_recovery_pair(
        "rust-ground-python-air-recovery",
        python_air_recovery_cmd(16070, 16070, 16170),
        rust_ground_cmd("127.0.0.1", 16170, 16071, 16071, 16171),
        rust_ground_cmd("127.0.0.1", 16170, 16072, 16072, 16172),
        16170,
    )
    run_pair(
        "rust-ground-python-air-restart",
        python_air_restart_cmd(16080, 16080, 16180),
        rust_ground_recovery_cmd("127.0.0.1", 16180, 16081, 16081, 16181),
        16180,
    )
    run_pair(
        "python-ground-python-air-competition",
        python_air_competition_cmd(16090, 16090, 16190),
        python_ground_competition_cmd(
            "127.0.0.1",
            16190,
            16091,
            16091,
            16191,
            16092,
            16092,
            16192,
        ),
        16190,
    )
    run_pair(
        "python-ground-python-air-dual-uav",
        python_air_dual_uav_cmd(16100, 16100, 16200, 16101, 16101, 16201),
        python_ground_dual_uav_cmd("127.0.0.1", 16200, 16201, 16102, 16102, 16202),
        16200,
    )

    print("[interop-matrix] OK")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
