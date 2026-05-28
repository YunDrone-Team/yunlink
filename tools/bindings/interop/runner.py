from __future__ import annotations

import argparse
import os
import subprocess
import time

from .commands import (
    cxx_air_cmd,
    python_air_cmd,
    python_air_competition_cmd,
    python_air_dual_uav_cmd,
    python_air_recovery_cmd,
    python_air_restart_cmd,
    python_ground_cmd,
    python_ground_competition_cmd,
    python_ground_dual_uav_cmd,
    rust_air_cmd,
    rust_ground_cmd,
    rust_ground_recovery_cmd,
)
from .paths import ROOT_DIR, RUST_BUILD_DIR
from .process import ManagedProc, ProcResult, require_ok, wait_for_tcp_listener


def runtime_env() -> dict[str, str]:
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
    return env


def run_pair(name: str, air_cmd: list[str], ground_cmd: list[str], air_port: int) -> None:
    print(f"== {name} ==")
    env = runtime_env()
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
    env = runtime_env()
    air = ManagedProc(f"{name}-air", air_cmd, ROOT_DIR, env)
    air.start()
    try:
        wait_for_tcp_listener("127.0.0.1", air_port, timeout_s=2.5)
        for index, ground_cmd in enumerate((first_ground_cmd, second_ground_cmd), start=1):
            ground = subprocess.run(
                ground_cmd,
                cwd=ROOT_DIR,
                env=env,
                text=True,
                capture_output=True,
                check=False,
                timeout=10.0,
            )
            require_ok(
                ProcResult(
                    f"{name}-ground-{index}",
                    ground.returncode,
                    ground.stdout,
                    ground.stderr,
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
